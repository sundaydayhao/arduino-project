/**
 * IMU 运动仪表盘 - MCU 端
 * Arduino UNO Q (Qualcomm)
 * 
 * MCU 读取 MPU6050 六轴数据，
 * 计算姿态角（Roll/Pitch/Yaw），
 * 通过 Bridge 发送到 Linux MPU，
 * 在浏览器中实时渲染 3D 姿态可视化。
 * 
 * 硬件：MPU6050 -> I2C (SDA=A4, SCL=A5)
 */

#include <Arduino.h>
#include <Arduino_RouterBridge.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;

// ===== 卡尔曼滤波器 =====
struct Kalman {
  float Q_angle = 0.001f;
  float Q_bias  = 0.003f;
  float R_measure = 0.03f;
  float angle = 0, bias = 0;
  float P[2][2] = {{0,0},{0,0}};

  float update(float newAngle, float newRate, float dt) {
    float rate = newRate - bias;
    angle += dt * rate;
    P[0][0] += dt * (dt*P[1][1] - P[0][1] - P[1][0] + Q_angle);
    P[0][1] -= dt * P[1][1];
    P[1][0] -= dt * P[1][1];
    P[1][1] += Q_bias * dt;
    float S = P[0][0] + R_measure;
    float K[2] = {P[0][0]/S, P[1][0]/S};
    float y = newAngle - angle;
    angle += K[0] * y;
    bias  += K[1] * y;
    P[0][0] -= K[0] * P[0][0];
    P[0][1] -= K[0] * P[0][1];
    P[1][0] -= K[1] * P[0][0];
    P[1][1] -= K[1] * P[0][1];
    return angle;
  }
} kalmanRoll, kalmanPitch;

float yaw = 0;
unsigned long lastTime = 0;

// ===== 计算姿态角 =====
void computeAngles(float ax, float ay, float az,
                   float gx, float gy, float gz,
                   float& roll, float& pitch) {
  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0f;
  lastTime = now;
  if (dt <= 0 || dt > 0.5f) dt = 0.01f;

  // 加速度计角度
  float accRoll  = atan2(ay, az) * 180.0f / PI;
  float accPitch = atan2(-ax, sqrt(ay*ay + az*az)) * 180.0f / PI;

  // 卡尔曼滤波融合
  roll  = kalmanRoll.update(accRoll,  gx, dt);
  pitch = kalmanPitch.update(accPitch, gy, dt);

  // Yaw 积分（无磁力计，会漂移）
  yaw += gz * dt;
  if (yaw >  180) yaw -= 360;
  if (yaw < -180) yaw += 360;
}

void setup() {
  Bridge.begin();
  Monitor.begin();
  Wire.begin();

  if (!mpu.begin()) {
    Monitor.println("MPU6050 未找到!");
    while (1) delay(100);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  lastTime = millis();
  Monitor.println("IMU 仪表盘已启动");
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float roll, pitch;
  computeAngles(
    a.acceleration.x, a.acceleration.y, a.acceleration.z,
    g.gyro.x * 180.0f / PI,
    g.gyro.y * 180.0f / PI,
    g.gyro.z * 180.0f / PI,
    roll, pitch
  );

  // 计算合加速度（检测运动强度）
  float accelMag = sqrt(
    a.acceleration.x * a.acceleration.x +
    a.acceleration.y * a.acceleration.y +
    a.acceleration.z * a.acceleration.z
  );

  // 通过 Bridge 发送到 Linux MPU
  Bridge.call("update_imu",
              roll, pitch, yaw,
              a.acceleration.x, a.acceleration.y, a.acceleration.z,
              g.gyro.x, g.gyro.y, g.gyro.z,
              temp.temperature,
              accelMag);

  delay(50);  // 20Hz 更新率
}