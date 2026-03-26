/*
 * 实验名称：DHT11 温湿度传感器
 * 实验目的：读取环境温度和湿度
 * 
 * 需要安装库：DHT sensor library（by Adafruit）
 * 安装方法：Arduino IDE → 工具 → 管理库 → 搜索 "DHT sensor library"
 * 同时需要安装：Adafruit Unified Sensor
 * 
 * 硬件连接：
 *   VCC → 5V（或3.3V）
 *   DATA → D2（需要 4.7kΩ 上拉电阻到 VCC）
 *   GND → GND
 * 
 * DHT11 规格：
 *   温度范围：0~50°C，精度 ±2°C
 *   湿度范围：20~90%RH，精度 ±5%RH
 *   采样率：1次/秒
 */

#include <DHT.h>

#define DHT_PIN  2
#define DHT_TYPE DHT11  // 使用 DHT22 时改为 DHT22

DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  Serial.begin(9600);
  dht.begin();
  
  Serial.println("DHT11 温湿度传感器实验");
  Serial.println("时间(s) | 温度(°C) | 湿度(%RH) | 热指数(°C)");
  Serial.println("--------|----------|-----------|----------");
}

void loop() {
  // DHT11 采样间隔至少 2 秒
  delay(2000);
  
  // 读取湿度
  float humidity = dht.readHumidity();
  // 读取温度（摄氏度）
  float temperature = dht.readTemperature();
  // 读取温度（华氏度）
  // float tempF = dht.readTemperature(true);
  
  // 检查读取是否成功
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("读取失败！请检查传感器连接");
    return;
  }
  
  // 计算热指数（体感温度）
  float heatIndex = dht.computeHeatIndex(temperature, humidity, false);
  
  // 输出数据
  Serial.print(millis() / 1000);
  Serial.print("s\t| ");
  Serial.print(temperature, 1);
  Serial.print("°C\t| ");
  Serial.print(humidity, 1);
  Serial.print("%\t| ");
  Serial.print(heatIndex, 1);
  Serial.println("°C");
}