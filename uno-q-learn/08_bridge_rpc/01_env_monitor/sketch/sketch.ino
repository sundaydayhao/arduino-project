/**
 * 环境监测站 - MCU 端
 * Arduino UNO Q (Qualcomm)
 * 
 * 架构：
 *   MCU 采集传感器数据
 *   → Bridge.call() 发送到 Linux MPU
 *   → Python REST API
 *   → 浏览器 Web 仪表盘
 * 
 * 传感器：
 *   DHT22  -> D2  (温湿度)
 *   BMP280 -> I2C (气压/海拔)
 *   MQ-135 -> A0  (空气质量)
 *   BH1750 -> I2C (光照强度)
 */

#include <Arduino.h>
#include <Arduino_RouterBridge.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <BH1750.h>

// ===== 传感器初始化 =====
#define DHT_PIN  2
#define DHT_TYPE DHT22
DHT           dht(DHT_PIN, DHT_TYPE);
Adafruit_BMP280 bmp;
BH1750        lightMeter;

// ===== 采样间隔 =====
constexpr unsigned long SAMPLE_INTERVAL_MS = 2000;

// ===== 读取空气质量（MQ-135）=====
// 返回 AQI 近似值（0-500）
int readAQI() {
  int raw = analogRead(A0);
  // 简单线性映射（实际应用需校准曲线）
  return map(raw, 0, 1023, 0, 500);
}

// ===== 读取所有传感器 =====
struct SensorData {
  float temperature;
  float humidity;
  float pressure;
  float altitude;
  float light;
  int   aqi;
  bool  valid;
};

SensorData readSensors() {
  SensorData d;
  d.temperature = dht.readTemperature();
  d.humidity    = dht.readHumidity();
  d.pressure    = bmp.readPressure() / 100.0f;  // Pa → hPa
  d.altitude    = bmp.readAltitude(1013.25f);
  d.light       = lightMeter.readLightLevel();
  d.aqi         = readAQI();
  d.valid       = !isnan(d.temperature) && !isnan(d.humidity);
  return d;
}

void setup() {
  Bridge.begin();
  Monitor.begin();

  Wire.begin();
  dht.begin();

  if (!bmp.begin(0x76)) {
    Monitor.println("BMP280 未找到，使用模拟数据");
  }
  if (!lightMeter.begin()) {
    Monitor.println("BH1750 未找到，使用模拟数据");
  }

  Monitor.println("环境监测站已启动");
  Monitor.println("等待传感器稳定...");
  delay(2000);
}

void loop() {
  static unsigned long lastSample = 0;

  if (millis() - lastSample >= SAMPLE_INTERVAL_MS) {
    lastSample = millis();

    SensorData d = readSensors();

    if (d.valid) {
      // 通过 Bridge 调用 Linux MPU 上的 Python 函数
      Bridge.call("update_env",
                  d.temperature,
                  d.humidity,
                  d.pressure,
                  d.altitude,
                  d.light,
                  d.aqi);

      Monitor.print("T:");    Monitor.print(d.temperature);
      Monitor.print(" H:");   Monitor.print(d.humidity);
      Monitor.print(" P:");   Monitor.print(d.pressure);
      Monitor.print(" L:");   Monitor.print(d.light);
      Monitor.print(" AQI:"); Monitor.println(d.aqi);
    } else {
      Monitor.println("传感器读取失败，跳过");
    }
  }
}