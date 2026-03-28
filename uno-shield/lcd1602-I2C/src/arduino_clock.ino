/**
 * ============================================================
 * Arduino R3 时钟项目
 * Arduino R3 Clock Project
 * ============================================================
 * 
 * 硬件配置 / Hardware:
 *   - Arduino UNO R3
 *   - DS3231 RTC 模块 (I2C: SDA=A4, SCL=A5)
 *   - LCD1602 + I2C 转接板 PCF8574 (I2C: SDA=A4, SCL=A5)
 * 
 * 所需库 / Required Libraries:
 *   - RTClib by Adafruit (搜索 "RTClib" 安装)
 *   - LiquidCrystal_I2C by Frank de Brabander (搜索 "LiquidCrystal I2C" 安装)
 * 
 * LCD 显示格式 / LCD Display Format:
 *   第1行: YYYY-MM-DD Day
 *   第2行: HH:MM:SS  TT.TC
 * 
 * I2C 地址 / I2C Addresses:
 *   DS3231 RTC  : 0x68 (固定)
 *   LCD PCF8574 : 0x27 (默认，部分模块为 0x3F)
 * 
 * 串口设置时间 / Set Time via Serial:
 *   发送 "T2024-03-25 12:30:00" 可手动设置时间
 * ============================================================
 */

#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>

// ---- 配置参数 / Configuration ----
// LCD I2C 地址，如显示乱码请改为 0x3F
#define LCD_I2C_ADDR  0x27
#define LCD_COLS      16
#define LCD_ROWS      2

// 串口波特率
#define SERIAL_BAUD   9600

// ---- 对象实例化 / Object Instantiation ----
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);
RTC_DS3231 rtc;

// ---- 星期名称 / Day Names ----
const char* dayNames[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// ---- 自定义字符：摄氏度符号 / Custom Char: Degree Symbol ----
byte degreeChar[8] = {
  0b00110,
  0b01001,
  0b01001,
  0b00110,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

// ---- 全局变量 ----
bool rtcOK = false;

// ============================================================
// 初始化 / Setup
// ============================================================
void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println(F("============================="));
  Serial.println(F("  Arduino R3 Clock Starting  "));
  Serial.println(F("============================="));

  // 初始化 LCD
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, degreeChar);
  lcd.clear();

  // 显示启动画面
  lcd.setCursor(0, 0);
  lcd.print(F(" Arduino  Clock "));
  lcd.setCursor(0, 1);
  lcd.print(F("  Initializing.."));
  delay(1500);

  // 初始化 RTC
  if (!rtc.begin()) {
    rtcOK = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("  RTC  ERROR!   "));
    lcd.setCursor(0, 1);
    lcd.print(F("Check Connection"));
    Serial.println(F("[ERROR] DS3231 RTC not found! Check wiring."));
    Serial.println(F("  SDA -> A4 (Arduino)"));
    Serial.println(F("  SCL -> A5 (Arduino)"));
    Serial.println(F("  VCC -> 5V"));
    Serial.println(F("  GND -> GND"));
    // 不进入死循环，允许继续运行以便调试
    return;
  }

  rtcOK = true;
  Serial.println(F("[OK] DS3231 RTC initialized."));

  // 如果 RTC 掉电，使用编译时间自动校时
  if (rtc.lostPower()) {
    Serial.println(F("[INFO] RTC lost power. Setting compile time..."));
    // 使用编译时间设置 RTC（注意：编译时间为本地时间）
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Time Auto-Reset!"));
    lcd.setCursor(0, 1);
    lcd.print(F("Use Build Time  "));
    delay(2000);
  }

  // 禁用 DS3231 的 32kHz 输出以节省功耗（可选）
  rtc.disable32K();

  lcd.clear();
  Serial.println(F("[OK] Clock ready!"));
  Serial.println(F("[TIP] Send 'T2024-03-25 12:30:00' to set time."));
  Serial.println(F("[TIP] Send 'R' to read current time."));
}

// ============================================================
// 主循环 / Main Loop
// ============================================================
void loop() {
  // 处理串口命令
  handleSerial();

  if (!rtcOK) {
    // RTC 未就绪时显示错误
    lcd.setCursor(0, 0);
    lcd.print(F("  RTC  ERROR!   "));
    lcd.setCursor(0, 1);
    lcd.print(F("Check Connection"));
    delay(1000);
    return;
  }

  // 读取当前时间
  DateTime now = rtc.now();

  // 读取 DS3231 内置温度传感器
  float temperature = rtc.getTemperature();

  // ---- 第1行：日期 + 星期 ----
  // 格式: "2024-03-25 Mon"  (14字符)
  char line1[17];
  snprintf(line1, sizeof(line1), "%04d-%02d-%02d %s",
           now.year(),
           now.month(),
           now.day(),
           dayNames[now.dayOfTheWeek()]);

  lcd.setCursor(0, 0);
  lcd.print(line1);

  // ---- 第2行：时间 + 温度 ----
  // 格式: "12:34:56  25.5C"  (15字符)
  char line2[17];
  int tempInt  = (int)temperature;
  int tempFrac = abs((int)((temperature - (float)tempInt) * 10.0f));

  snprintf(line2, sizeof(line2), "%02d:%02d:%02d %3d.%1dC",
           now.hour(),
           now.minute(),
           now.second(),
           tempInt,
           tempFrac);

  lcd.setCursor(0, 1);
  lcd.print(line2);

  // 串口输出调试信息
  Serial.print(F("[TIME] "));
  Serial.print(line1);
  Serial.print(F("  "));
  Serial.println(line2);

  delay(1000);
}

// ============================================================
// 串口命令处理 / Serial Command Handler
// 支持手动设置时间：发送 "T2024-03-25 12:30:00"
// ============================================================
void handleSerial() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd.length() == 0) return;

  // 命令 'R': 读取并打印当前时间
  if (cmd == "R" || cmd == "r") {
    if (!rtcOK) {
      Serial.println(F("[ERROR] RTC not available."));
      return;
    }
    DateTime now = rtc.now();
    char buf[32];
    snprintf(buf, sizeof(buf), "[RTC] %04d-%02d-%02d %02d:%02d:%02d %s",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second(),
             dayNames[now.dayOfTheWeek()]);
    Serial.println(buf);
    Serial.print(F("[TEMP] "));
    Serial.print(rtc.getTemperature());
    Serial.println(F(" C"));
    return;
  }

  // 命令 'T': 设置时间，格式 "T2024-03-25 12:30:00"
  if (cmd.charAt(0) == 'T' || cmd.charAt(0) == 't') {
    if (cmd.length() < 20) {
      Serial.println(F("[ERROR] Format: T2024-03-25 12:30:00"));
      return;
    }
    // 解析: T YYYY-MM-DD HH:MM:SS
    //        0 1234567890123456789
    int year   = cmd.substring(1, 5).toInt();
    int month  = cmd.substring(6, 8).toInt();
    int day    = cmd.substring(9, 11).toInt();
    int hour   = cmd.substring(12, 14).toInt();
    int minute = cmd.substring(15, 17).toInt();
    int second = cmd.substring(18, 20).toInt();

    if (year < 2000 || year > 2099 ||
        month < 1 || month > 12 ||
        day < 1 || day > 31 ||
        hour > 23 || minute > 59 || second > 59) {
      Serial.println(F("[ERROR] Invalid date/time values."));
      return;
    }

    if (!rtcOK) {
      Serial.println(F("[ERROR] RTC not available."));
      return;
    }

    rtc.adjust(DateTime(year, month, day, hour, minute, second));
    Serial.print(F("[OK] Time set to: "));
    Serial.print(year); Serial.print('-');
    if (month < 10) Serial.print('0');
    Serial.print(month); Serial.print('-');
    if (day < 10) Serial.print('0');
    Serial.print(day); Serial.print(' ');
    if (hour < 10) Serial.print('0');
    Serial.print(hour); Serial.print(':');
    if (minute < 10) Serial.print('0');
    Serial.print(minute); Serial.print(':');
    if (second < 10) Serial.print('0');
    Serial.println(second);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Time Updated!   "));
    lcd.setCursor(0, 1);
    lcd.print(F("Via Serial Port "));
    delay(1500);
    lcd.clear();
    return;
  }

  Serial.println(F("[HELP] Commands:"));
  Serial.println(F("  R              - Read current time"));
  Serial.println(F("  T2024-03-25 12:30:00 - Set time"));
}