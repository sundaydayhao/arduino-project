/*
 * 实验名称：DHT11 + LCD1602 温湿度显示
 * 实验目的：将温湿度数据显示在 LCD 屏幕上
 * 
 * 需要安装库：
 *   - DHT sensor library（Adafruit）
 *   - LiquidCrystal_I2C
 * 
 * 硬件连接：
 *   DHT11: DATA → D2
 *   LCD1602 I2C: SDA → A4, SCL → A5
 *   （I2C 地址通常为 0x27 或 0x3F）
 */

#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define DHT_PIN  2
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // 地址0x27，16列2行

// 自定义字符：温度计图标
byte thermometer[8] = {
  0b00100,
  0b01010,
  0b01010,
  0b01110,
  0b01110,
  0b11111,
  0b11111,
  0b01110
};

// 自定义字符：水滴图标
byte droplet[8] = {
  0b00100,
  0b00100,
  0b01110,
  0b01110,
  0b11111,
  0b11111,
  0b11111,
  0b01110
};

void setup() {
  Serial.begin(9600);
  dht.begin();
  
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, thermometer);
  lcd.createChar(1, droplet);
  
  lcd.setCursor(0, 0);
  lcd.print("  温湿度监测仪  ");
  lcd.setCursor(0, 1);
  lcd.print("  正在初始化... ");
  delay(2000);
}

void loop() {
  delay(2000);
  
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  if (isnan(humidity) || isnan(temperature)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("传感器错误!");
    Serial.println("DHT 读取失败");
    return;
  }
  
  // 第一行：温度
  lcd.setCursor(0, 0);
  lcd.write(0);  // 温度计图标
  lcd.print(" Temp: ");
  lcd.print(temperature, 1);
  lcd.print((char)223);  // 度符号
  lcd.print("C  ");
  
  // 第二行：湿度
  lcd.setCursor(0, 1);
  lcd.write(1);  // 水滴图标
  lcd.print(" Humi: ");
  lcd.print(humidity, 1);
  lcd.print("%    ");
  
  Serial.print("温度: ");
  Serial.print(temperature);
  Serial.print("°C  湿度: ");
  Serial.print(humidity);
  Serial.println("%");
}