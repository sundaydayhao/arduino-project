/*
 * LCD12864 图形时钟 Shield
 * ========================
 * 功能：
 *   - 左侧：模拟时钟表盘（带时针/分针/秒针/刻度）
 *   - 右侧：数字时间、日期、星期、温度
 *   - DS3231 RTC 高精度计时（±2ppm）
 *   - 串口命令校时（格式：T2024-03-25 12:30:00）
 *
 * 硬件连接：
 *   LCD12864 (ST7920, 软件SPI模式, PSB接GND)
 *     SCLK (E)  -> Pin 8
 *     SID  (RW) -> Pin 9
 *     CS   (RS) -> Pin 10
 *     RST       -> Pin 7
 *     PSB       -> GND  ← 必须接GND才能进入SPI模式
 *     VCC       -> 5V
 *     GND       -> GND
 *     V0        -> 电位器中间脚（对比度调节）
 *     BLA       -> 5V（背光正极）
 *     BLK       -> GND（背光负极）
 *
 *   DS3231 RTC (I2C)
 *     SDA -> A4
 *     SCL -> A5
 *     VCC -> 5V
 *     GND -> GND
 *
 * 所需库（Arduino IDE 库管理器安装）：
 *   - U8g2  (by olikraus)
 *   - RTClib (by Adafruit)
 *
 * 串口命令（波特率 9600）：
 *   T2024-03-25 12:30:00   设置时间
 *   R                      读取当前时间
 */

#include <U8g2lib.h>
#include <Wire.h>
#include <RTClib.h>

// ─── 引脚定义 ────────────────────────────────────────────────
#define LCD_SCLK  8
#define LCD_SID   9
#define LCD_CS    10
#define LCD_RST   7

// ─── LCD 初始化（ST7920 软件SPI，全缓冲模式）────────────────
// U8G2_ST7920_128X64_F_SW_SPI(rotation, clock, data, cs [, reset])
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, LCD_SCLK, LCD_SID, LCD_CS, LCD_RST);

RTC_DS3231 rtc;

// ─── 常量 ────────────────────────────────────────────────────
const char* DAYS[]   = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char* MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

// 模拟时钟参数
#define CLOCK_CX  31    // 圆心 X
#define CLOCK_CY  32    // 圆心 Y
#define CLOCK_R   28    // 外圆半径

// ─── 工具函数 ─────────────────────────────────────────────────

// 角度转坐标（0° = 12点方向，顺时针）
void polarToXY(int cx, int cy, float radius, float angleDeg,
               int &x, int &y) {
  float rad = angleDeg * PI / 180.0;
  x = cx + (int)(radius * sin(rad));
  y = cy - (int)(radius * cos(rad));
}

// ─── 绘制模拟时钟 ─────────────────────────────────────────────
void drawAnalogClock(uint8_t hour, uint8_t minute, uint8_t second) {
  // 外圆
  u8g2.drawCircle(CLOCK_CX, CLOCK_CY, CLOCK_R);

  // 12 个小时刻度
  for (int i = 0; i < 12; i++) {
    float angle = i * 30.0;
    int x1, y1, x2, y2;
    polarToXY(CLOCK_CX, CLOCK_CY, CLOCK_R - 3, angle, x1, y1);
    polarToXY(CLOCK_CX, CLOCK_CY, CLOCK_R,     angle, x2, y2);
    u8g2.drawLine(x1, y1, x2, y2);
  }

  // 时针（长度 = R * 0.50）
  float hourAngle = (hour % 12) * 30.0 + minute * 0.5;
  int hx, hy;
  polarToXY(CLOCK_CX, CLOCK_CY, CLOCK_R * 0.50, hourAngle, hx, hy);
  u8g2.drawLine(CLOCK_CX, CLOCK_CY, hx, hy);
  // 加粗时针（偏移1像素再画一次）
  u8g2.drawLine(CLOCK_CX + 1, CLOCK_CY, hx, hy);

  // 分针（长度 = R * 0.75）
  float minAngle = minute * 6.0 + second * 0.1;
  int mx, my;
  polarToXY(CLOCK_CX, CLOCK_CY, CLOCK_R * 0.75, minAngle, mx, my);
  u8g2.drawLine(CLOCK_CX, CLOCK_CY, mx, my);

  // 秒针（长度 = R * 0.85，细线）
  float secAngle = second * 6.0;
  int sx, sy;
  polarToXY(CLOCK_CX, CLOCK_CY, CLOCK_R * 0.85, secAngle, sx, sy);
  u8g2.drawLine(CLOCK_CX, CLOCK_CY, sx, sy);

  // 中心圆点
  u8g2.drawDisc(CLOCK_CX, CLOCK_CY, 2);
}

// ─── 绘制右侧数字信息 ─────────────────────────────────────────
void drawDigitalInfo(const DateTime &now, float temp) {
  char buf[20];

  // 分隔竖线
  u8g2.drawVLine(64, 0, 64);

  // 数字时间（大字体）
  u8g2.setFont(u8g2_font_7x13B_tf);
  sprintf(buf, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  u8g2.drawStr(67, 14, buf);

  // 日期（小字体）
  u8g2.setFont(u8g2_font_6x10_tf);
  sprintf(buf, "%04d-%02d-%02d", now.year(), now.month(), now.day());
  u8g2.drawStr(67, 28, buf);

  // 星期
  sprintf(buf, "%s", DAYS[now.dayOfTheWeek()]);
  u8g2.drawStr(67, 40, buf);

  // 温度
  u8g2.setFont(u8g2_font_7x13B_tf);
  if (temp > -40.0) {
    sprintf(buf, "%.1f", temp);
    u8g2.drawStr(67, 56, buf);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(103, 56, "\xb0""C");   // °C
  } else {
    u8g2.drawStr(67, 56, "N/A");
  }
}

// ─── 串口校时 ─────────────────────────────────────────────────
// 格式：T2024-03-25 12:30:00
void handleSerial() {
  if (!Serial.available()) return;
  char cmd = Serial.read();

  if (cmd == 'R' || cmd == 'r') {
    DateTime now = rtc.now();
    char buf[30];
    sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d",
            now.year(), now.month(), now.day(),
            now.hour(), now.minute(), now.second());
    Serial.print("Current time: ");
    Serial.println(buf);
    return;
  }

  if (cmd == 'T' || cmd == 't') {
    // 等待完整字符串
    delay(100);
    String s = Serial.readStringUntil('\n');
    s.trim();
    // 格式：YYYY-MM-DD HH:MM:SS
    if (s.length() >= 19) {
      int yr  = s.substring(0,  4).toInt();
      int mo  = s.substring(5,  7).toInt();
      int dy  = s.substring(8,  10).toInt();
      int hr  = s.substring(11, 13).toInt();
      int mn  = s.substring(14, 16).toInt();
      int sc  = s.substring(17, 19).toInt();
      rtc.adjust(DateTime(yr, mo, dy, hr, mn, sc));
      Serial.println("Time updated OK");
    } else {
      Serial.println("Format: TYYYY-MM-DD HH:MM:SS");
    }
  }
}

// ─── Setup ────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  Serial.println("=== LCD12864 Graphic Clock ===");

  // 初始化 LCD
  u8g2.begin();

  // 初始化 RTC
  if (!rtc.begin()) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 20, "RTC not found!");
    u8g2.drawStr(0, 35, "Check wiring.");
    u8g2.sendBuffer();
    Serial.println("ERROR: RTC not found!");
    while (1) delay(1000);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting compile time...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  Serial.println("Ready. Commands: R=read time, T<YYYY-MM-DD HH:MM:SS>=set time");
}

// ─── Loop ─────────────────────────────────────────────────────
void loop() {
  handleSerial();

  DateTime now  = rtc.now();
  float    temp = rtc.getTemperature();

  u8g2.clearBuffer();
  drawAnalogClock(now.hour(), now.minute(), now.second());
  drawDigitalInfo(now, temp);
  u8g2.sendBuffer();

  delay(1000);
}