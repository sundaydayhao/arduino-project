/**
 * Matter 智能家居 - MCU 端
 * Arduino UNO Q (Qualcomm)
 *
 * 双脑架构：
 *   MCU (Cortex-M33) → Matter 协议栈，设备配对，硬件控制
 *   Linux MPU        → 自动化规则，Web 控制面板，REST API
 *
 * 实现的 Matter 设备类型：
 *   - 可调光灯（On/Off + 亮度 + 色温）
 *   - 风扇（On/Off + 转速）
 *   - 温湿度传感器
 *   - 占位传感器
 *
 * 依赖库：Arduino_Matter, Arduino_RouterBridge, Adafruit_BME280
 */

#include <Arduino_RouterBridge.h>
#include <Arduino_Matter.h>
#include <Wire.h>
#include <Adafruit_BME280.h>

// ===== 硬件引脚 =====
#define LED_PIN       5   // PWM 调光
#define FAN_PIN       6   // PWM 风扇
#define PIR_PIN       3   // PIR 占位传感器
#define BUTTON_PIN    2   // 配对按键

Adafruit_BME280 bme;

// ===== Matter 设备对象 =====
MatterDimmableLight  matterLight;
MatterFan            matterFan;
MatterTemperatureSensor matterTempSensor;
MatterHumiditySensor    matterHumiSensor;
MatterOccupancySensor   matterOccSensor;

// ===== 状态 =====
bool lightOn       = false;
int  lightBrightness = 100;
bool fanOn         = false;
int  fanSpeed      = 0;
bool occupancy     = false;

unsigned long lastSensorMs = 0;
constexpr unsigned long SENSOR_INTERVAL = 10000;  // 10 秒上报一次

// ===== Matter 回调 =====
void onLightChange(bool on, uint8_t brightness) {
  lightOn         = on;
  lightBrightness = brightness;
  analogWrite(LED_PIN, on ? map(brightness, 0, 100, 0, 255) : 0);
  Monitor.printf("[Matter] 灯: %s 亮度:%d%%\n", on ? "开" : "关", brightness);
  Bridge.call("update_device_state", "light", "on",         (int)on);
  Bridge.call("update_device_state", "light", "brightness", brightness);
}

void onFanChange(bool on, uint8_t speed) {
  fanOn    = on;
  fanSpeed = speed;
  analogWrite(FAN_PIN, on ? map(speed, 0, 100, 0, 255) : 0);
  Monitor.printf("[Matter] 风扇: %s 转速:%d%%\n", on ? "开" : "关", speed);
  Bridge.call("update_device_state", "fan", "on",    (int)on);
  Bridge.call("update_device_state", "fan", "speed", speed);
}

// ===== MPU 下发控制命令 =====
void set_device(const char* device, const char* key, float value) {
  String dev(device), k(key);
  Monitor.printf("[MPU->MCU] 设置 %s.%s = %.1f\n", device, key, value);

  if (dev == "light") {
    if (k == "on") {
      matterLight.setOnOff((bool)(int)value);
    } else if (k == "brightness") {
      matterLight.setBrightness((uint8_t)value);
    }
  } else if (dev == "fan") {
    if (k == "on") {
      matterFan.setOnOff((bool)(int)value);
    } else if (k == "speed") {
      matterFan.setSpeedPercent((uint8_t)value);
    }
  }
}

void setup() {
  Bridge.begin();
  Monitor.begin();

  Monitor.println("========================================");
  Monitor.println("  Arduino UNO Q - Matter 智能家居");
  Monitor.println("  MCU: Matter 协议 | MPU: 自动化+API");
  Monitor.println("========================================");

  pinMode(LED_PIN,    OUTPUT);
  pinMode(FAN_PIN,    OUTPUT);
  pinMode(PIR_PIN,    INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // 初始化 BME280
  if (!bme.begin(0x76) && !bme.begin(0x77)) {
    Monitor.println("[WARN] BME280 未找到，跳过温湿度传感器");
  } else {
    Monitor.println("[OK] BME280 初始化成功");
  }

  // 注册 Bridge 函数（MPU 下发控制）
  Bridge.provide("set_device", set_device);

  // 初始化 Matter
  Matter.begin();
  matterLight.begin();
  matterFan.begin();
  matterTempSensor.begin();
  matterHumiSensor.begin();
  matterOccSensor.begin();

  matterLight.onChangeCallback(onLightChange);
  matterFan.onChangeCallback(onFanChange);

  Monitor.println("[OK] Matter 初始化成功");
  Monitor.println("  长按 D2 (5s) 进入配对模式");

  if (!Matter.isCommissioned()) {
    Monitor.println("[Matter] 未配对，等待配对...");
    Monitor.printf("  配对码: %s\n", Matter.getManualPairingCode().c_str());
    Monitor.printf("  QR 码:  %s\n", Matter.getOnboardingQRCodeUrl().c_str());
  } else {
    Monitor.println("[Matter] 已配对，设备在线");
    Bridge.call("on_commissioned",
                Matter.getFabricId(),
                Matter.getNodeId());
  }
}

void loop() {
  Matter.loop();

  // ===== 长按按键重置配对 =====
  static unsigned long btnPressMs = 0;
  if (digitalRead(BUTTON_PIN) == LOW) {
    if (btnPressMs == 0) btnPressMs = millis();
    if (millis() - btnPressMs > 5000) {
      Monitor.println("[Matter] 重置配对信息...");
      Matter.decommission();
      btnPressMs = 0;
    }
  } else {
    btnPressMs = 0;
  }

  // ===== PIR 占位检测 =====
  bool newOcc = digitalRead(PIR_PIN) == HIGH;
  if (newOcc != occupancy) {
    occupancy = newOcc;
    matterOccSensor.setOccupancy(occupancy);
    Monitor.printf("[PIR] 占位: %s\n", occupancy ? "有人" : "无人");
    Bridge.call("update_device_state", "sensor", "occupancy", (int)occupancy);
  }

  // ===== 定期上报传感器数据 =====
  if (millis() - lastSensorMs >= SENSOR_INTERVAL) {
    lastSensorMs = millis();

    float temp = bme.readTemperature();
    float humi = bme.readHumidity();

    matterTempSensor.setTemperature(temp);
    matterHumiSensor.setHumidity(humi);

    Monitor.printf("[传感器] T:%.1fC H:%.1f%%\n", temp, humi);

    Bridge.call("update_device_state", "sensor", "temperature", temp);
    Bridge.call("update_device_state", "sensor", "humidity",    humi);
  }
}