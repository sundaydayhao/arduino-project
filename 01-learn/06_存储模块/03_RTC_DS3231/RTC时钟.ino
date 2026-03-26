/*
 * 实验名称：DS3231 RTC 实时时钟
 * 实验目的：读取精确时间，实现时钟功能
 * 
 * 需要安装库：RTClib（by Adafruit）
 * 
 * 硬件连接（I2C）：
 *   VCC → 3.3V 或 5V
 *   GND → GND
 *   SDA → A4
 *   SCL → A5
 * 
 * DS3231 特点：
 *   - 内置温度补偿晶振，精度极高（±2ppm）
 *   - 内置温度传感器
 *   - 备用电池（CR2032）保持时间
 */

#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

const char* DAYS[] = {"日", "一", "二", "三", "四", "五", "六"};
const char* MONTHS[] = {"", "1月", "2月", "3月", "4月", "5月", "6月",
                        "7月", "8月", "9月", "10月", "11月", "12月"};

void setup() {
  Serial.begin(9600);
  Wire.begin();
  
  if (!rtc.begin()) {
    Serial.println("DS3231 未找到！检查接线");
    while (true);
  }
  
  // 如果时钟停止（电池耗尽），重新设置时间
  if (rtc.lostPower()) {
    Serial.println("RTC 时钟停止，正在设置时间...");
    // 设置为编译时间（上传时自动设置）
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  Serial.println("DS3231 RTC 实时时钟");
  Serial.println("命令: set YYYY-MM-DD HH:MM:SS | time | temp");
}

void loop() {
  // 检查串口命令
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    if (cmd.startsWith("set ")) {
      // 格式: set 2024-01-15 14:30:00
      String dateTime = cmd.substring(4);
      int year  = dateTime.substring(0, 4).toInt();
      int month = dateTime.substring(5, 7).toInt();
      int day   = dateTime.substring(8, 10).toInt();
      int hour  = dateTime.substring(11, 13).toInt();
      int min   = dateTime.substring(14, 16).toInt();
      int sec   = dateTime.substring(17, 19).toInt();
      
      rtc.adjust(DateTime(year, month, day, hour, min, sec));
      Serial.println("时间已设置！");
    }
    else if (cmd == "temp") {
      Serial.print("DS3231 温度: ");
      Serial.print(rtc.getTemperature());
      Serial.println("°C");
    }
  }
  
  // 每秒显示时间
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();
    printTime();
  }
}

void printTime() {
  DateTime now = rtc.now();
  
  Serial.print(now.year());
  Serial.print("-");
  if (now.month() < 10) Serial.print("0");
  Serial.print(now.month());
  Serial.print("-");
  if (now.day() < 10) Serial.print("0");
  Serial.print(now.day());
  Serial.print(" 周");
  Serial.print(DAYS[now.dayOfTheWeek()]);
  Serial.print(" ");
  if (now.hour() < 10) Serial.print("0");
  Serial.print(now.hour());
  Serial.print(":");
  if (now.minute() < 10) Serial.print("0");
  Serial.print(now.minute());
  Serial.print(":");
  if (now.second() < 10) Serial.print("0");
  Serial.println(now.second());
}