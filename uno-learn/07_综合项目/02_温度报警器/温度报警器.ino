/*
 * 综合项目：温度报警器
 * 功能：
 *   1. DHT11 读取温湿度
 *   2. LCD1602 显示数据
 *   3. 超过阈值蜂鸣报警
 *   4. LED 状态指示
 *   5. 串口记录数据
 * 
 * 需要安装库：DHT sensor library, LiquidCrystal_I2C
 * 
 * 硬件：
 *   DHT11 → D2
 *   LCD I2C → A4(SDA), A5(SCL)
 *   蜂鸣器 → D8
 *   绿灯 → D6，红灯 → D7
 */

#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define DHT_PIN  2
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int BUZZER    = 8;
const int LED_GREEN = 6;
const int LED_RED   = 7;

// 报警阈值
float TEMP_HIGH = 30.0;  // 高温报警
float TEMP_LOW  = 10.0;  // 低温报警
float HUMI_HIGH = 80.0;  // 高湿报警

bool alarmActive = false;

void setup() {
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  
  dht.begin();
  lcd.init();
  lcd.backlight();
  Serial.begin(9600);
  
  lcd.setCursor(0, 0);
  lcd.print("  温度报警系统  ");
  lcd.setCursor(0, 1);
  lcd.print("  正在初始化... ");
  delay(2000);
  
  Serial.println("温度报警器启动");
  Serial.print("高温阈值: "); Serial.print(TEMP_HIGH); Serial.println("°C");
  Serial.print("低温阈值: "); Serial.print(TEMP_LOW); Serial.println("°C");
}

void loop() {
  delay(2000);
  
  float temp = dht.readTemperature();
  float humi = dht.readHumidity();
  
  if (isnan(temp) || isnan(humi)) {
    lcd.clear();
    lcd.print("传感器错误!");
    return;
  }
  
  // 判断报警条件
  bool tempAlarm = (temp > TEMP_HIGH || temp < TEMP_LOW);
  bool humiAlarm = (humi > HUMI_HIGH);
  alarmActive = tempAlarm || humiAlarm;
  
  // 更新 LCD
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temp, 1);
  lcd.print((char)223);
  lcd.print("C H:");
  lcd.print(humi, 0);
  lcd.print("%  ");
  
  lcd.setCursor(0, 1);
  if (alarmActive) {
    lcd.print("!! 警报 !!");
    if (tempAlarm) lcd.print(temp > TEMP_HIGH ? "高温" : "低温");
    if (humiAlarm) lcd.print("高湿");
  } else {
    lcd.print("状态: 正常      ");
  }
  
  // LED 和蜂鸣器
  if (alarmActive) {
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
    tone(BUZZER, 2000, 500);
  } else {
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, HIGH);
    noTone(BUZZER);
  }
  
  // 串口记录
  Serial.print(millis()/1000);
  Serial.print("s T=");
  Serial.print(temp, 1);
  Serial.print("°C H=");
  Serial.print(humi, 1);
  Serial.print("% ");
  Serial.println(alarmActive ? "⚠️ 报警!" : "正常");
}