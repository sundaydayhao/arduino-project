/*
 * 综合项目：超声波避障小车
 * 功能：
 *   - 超声波检测前方障碍物
 *   - 遇到障碍物自动转向
 *   - 舵机转动超声波扫描左右
 * 
 * 需要安装库：Servo
 * 
 * 硬件：
 *   HC-SR04: Trig→D7, Echo→D8
 *   舵机（超声波云台）→ D6
 *   左电机: IN1→D2, IN2→D3(PWM)
 *   右电机: IN3→D4(PWM), IN4→D5
 *   电源: 7.4V 锂电池
 */

#include <Servo.h>

// 超声波
const int TRIG = 7;
const int ECHO = 8;

// 舵机
Servo scanServo;
const int SERVO_PIN = 6;

// 电机
const int L_IN1 = 2, L_IN2 = 3;
const int R_IN3 = 4, R_IN4 = 5;

const int CAR_SPEED   = 150;
const float SAFE_DIST = 20.0;  // 安全距离 20cm

void setup() {
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(L_IN1, OUTPUT); pinMode(L_IN2, OUTPUT);
  pinMode(R_IN3, OUTPUT); pinMode(R_IN4, OUTPUT);
  
  scanServo.attach(SERVO_PIN);
  scanServo.write(90);  // 正前方
  
  Serial.begin(9600);
  Serial.println("避障小车启动！");
  delay(2000);
}

void loop() {
  float dist = getDistance();
  Serial.print("前方距离: ");
  Serial.print(dist);
  Serial.println("cm");
  
  if (dist > SAFE_DIST) {
    forward();
  } else {
    stopCar();
    delay(300);
    
    // 扫描左右
    float leftDist  = scanDirection(150);
    float rightDist = scanDirection(30);
    scanServo.write(90);
    delay(300);
    
    Serial.print("左: "); Serial.print(leftDist);
    Serial.print("cm 右: "); Serial.println(rightDist);
    
    if (leftDist > rightDist) {
      turnLeft();
      delay(500);
    } else {
      turnRight();
      delay(500);
    }
    stopCar();
  }
  delay(100);
}

float scanDirection(int angle) {
  scanServo.write(angle);
  delay(500);
  return getDistance();
}

float getDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long dur = pulseIn(ECHO, HIGH, 30000);
  if (dur == 0) return 999;
  return dur * 0.034 / 2.0;
}

void forward() {
  analogWrite(L_IN1, CAR_SPEED); digitalWrite(L_IN2, LOW);
  analogWrite(R_IN3, CAR_SPEED); digitalWrite(R_IN4, LOW);
}
void turnLeft() {
  digitalWrite(L_IN1, LOW); analogWrite(L_IN2, CAR_SPEED);
  analogWrite(R_IN3, CAR_SPEED); digitalWrite(R_IN4, LOW);
}
void turnRight() {
  analogWrite(L_IN1, CAR_SPEED); digitalWrite(L_IN2, LOW);
  digitalWrite(R_IN3, LOW); analogWrite(R_IN4, CAR_SPEED);
}
void stopCar() {
  digitalWrite(L_IN1, LOW); digitalWrite(L_IN2, LOW);
  digitalWrite(R_IN3, LOW); digitalWrite(R_IN4, LOW);
}