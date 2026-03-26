/**
 * 传感器数据 AI 分析 - MCU 端
 * Arduino UNO Q (Qualcomm)
 *
 * 双脑架构：
 *   MCU (Cortex-M33) → 传感器采集（温湿度/气压/IMU），定期上报
 *   Linux MPU        → 调用 LLM 分析传感器数据，生成洞察报告
 *
 * 传感器：BME280 (温湿度气压) + LSM6DS3 (IMU)
 * 上报频率：每 5 秒一次
 *
 * 依赖库：Adafruit_BME280, Arduino_LSM6DS3, Arduino_RouterBridge
 */

#include <Arduino_RouterBridge.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Arduino_LSM6DS3.h>

Adafruit_BME280 bme;

// ===== 配置 =====
constexpr unsigned long REPORT_INTERVAL_MS = 5000;  // 5 秒上报一次
unsigned long lastReportMs = 0;
int sampleCount = 0;

void setup() {
  Bridge.begin();
  Monitor.begin();

  Monitor.println("========================================");
  Monitor.println("  Arduino UNO Q - 传感器 AI 分析");
  Monitor.println("  MCU: 传感器采集 | MPU: LLM 分析");
  Monitor.println("========================================");

  // 初始化 BME280
  if (!bme.begin(0x76)) {
    if (!bme.begin(0x77)) {
      Monitor.println("[ERROR] BME280 初始化失败! 检查 I2C 接线");
      while (1) delay(1000);
    }
  }
  Monitor.println("[OK] BME280 初始化成功");

  // 初始化 IMU
  if (!IMU.begin()) {
    Monitor.println("[WARN] IMU 初始化失败，将跳过 IMU 数据");
  } else {
    Monitor.println("[OK] IMU 初始化成功");
  }

  Monitor.printf("[OK] 系统就绪，每 %lu 秒上报一次传感器数据\n\n",
                REPORT_INTERVAL_MS / 1000);
}

void loop() {
  if (millis() - lastReportMs < REPORT_INTERVAL_MS) return;
  lastReportMs = millis();
  sampleCount++;

  // 读取 BME280
  float temperature = bme.readTemperature();
  float humidity    = bme.readHumidity();
  float pressure    = bme.readPressure() / 100.0f;  // Pa -> hPa

  // 读取 IMU
  float ax = 0, ay = 0, az = 0;
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(ax, ay, az);
  }

  Monitor.printf("[%4d] T:%.1fC H:%.1f%% P:%.1fhPa IMU:[%.2f,%.2f,%.2f]\n",
                sampleCount, temperature, humidity, pressure, ax, ay, az);

  // ===== 上报传感器数据到 Linux MPU =====
  // MPU 端 main.py 的 update_sensors() 函数接收并积累数据
  // 每 30 条数据触发一次 LLM 分析
  Bridge.call("update_sensors",
              temperature, humidity, pressure,
              ax, ay, az);

  // 每 30 次采样显示一次 MPU 的分析结果
  if (sampleCount % 30 == 0) {
    Monitor.println("\n[等待 AI 分析结果...]");
    // 分析在 MPU 后台异步进行，结果可通过 Web API 查看
    // http://<board-ip>/api/state -> last_analysis 字段
  }
}