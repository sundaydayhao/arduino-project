/**
 * 工业多路传感器监控 - MCU 端
 * Arduino UNO Q (Qualcomm)
 *
 * 模拟工业场景：
 *   - 4路模拟量采集（电流/电压/振动/温度）
 *   - 转速计数（霍尔传感器）
 *   - 数字量 I/O 状态
 *   - 通过 Bridge 发送到 Linux MPU 做数据处理
 *   - Linux MPU 计算 FFT、统计、告警
 *
 * 硬件：
 *   A0 - 电流传感器 ACS712
 *   A1 - 振动传感器 ADXL335
 *   A2 - 热电偶 MAX6675
 *   A3 - 压力传感器 0-5V
 *   D3 - 霍尔传感器（转速）
 *   D4,D5,D6 - 数字量输入（限位开关等）
 */

#include <Arduino.h>
#include <Arduino_RouterBridge.h>

// ===== 霍尔传感器（转速）=====
volatile unsigned long hallCount = 0;
unsigned long lastRpmCalc = 0;
float rpm = 0;

void IRAM_ATTR hallISR() {
  hallCount++;
}

// ===== 读取电流（ACS712 30A）=====
float readCurrent_A() {
  int raw = analogRead(A0);
  float voltage = raw * 5.0f / 1023.0f;
  return (voltage - 2.5f) / 0.066f;  // ACS712-30A: 66mV/A
}

// ===== 读取振动 RMS =====
float readVibration_g() {
  // 采集 64 个样本计算 RMS
  long sum = 0;
  for (int i = 0; i < 64; i++) {
    int v = analogRead(A1) - 512;
    sum += (long)v * v;
    delayMicroseconds(100);
  }
  float rms = sqrt((float)sum / 64.0f);
  return rms * 3.3f / 1023.0f / 0.3f;  // 转换为 g
}

// ===== 读取温度（MAX6675 热电偶）=====
float readTemperature_C() {
  // 简化版：实际应使用 SPI 读取 MAX6675
  int raw = analogRead(A2);
  return raw * 500.0f / 1023.0f;  // 0-500°C 映射
}

// ===== 读取压力 =====
float readPressure_bar() {
  int raw = analogRead(A3);
  return raw * 10.0f / 1023.0f;  // 0-10 bar
}

// ===== 计算转速 =====
float calcRPM() {
  unsigned long now = millis();
  unsigned long dt = now - lastRpmCalc;
  if (dt >= 1000) {
    noInterrupts();
    unsigned long count = hallCount;
    hallCount = 0;
    interrupts();
    rpm = (count * 60000.0f) / dt;  // 每分钟转数
    lastRpmCalc = now;
  }
  return rpm;
}

// ===== 读取数字量 =====
uint8_t readDigitalInputs() {
  uint8_t state = 0;
  if (digitalRead(4)) state |= 0x01;
  if (digitalRead(5)) state |= 0x02;
  if (digitalRead(6)) state |= 0x04;
  return state;
}

void setup() {
  Bridge.begin();
  Monitor.begin();

  pinMode(3, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(3), hallISR, RISING);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);

  lastRpmCalc = millis();
  Monitor.println("工业监控系统已启动");
}

void loop() {
  static unsigned long lastSend = 0;

  if (millis() - lastSend >= 500) {  // 2Hz 上报
    lastSend = millis();

    float current   = readCurrent_A();
    float vibration = readVibration_g();
    float temp      = readTemperature_C();
    float pressure  = readPressure_bar();
    float speed     = calcRPM();
    int   dio       = readDigitalInputs();

    // 计算功率（假设 220V）
    float power = abs(current) * 220.0f;

    Bridge.call("update_industrial",
                current, vibration, temp, pressure,
                speed, power, dio,
                (long)millis());

    Monitor.printf("I=%.2fA V=%.3fg T=%.1fC P=%.2fbar RPM=%.0f W=%.0f DIO=%d\n",
                   current, vibration, temp, pressure, speed, power, dio);
  }
}