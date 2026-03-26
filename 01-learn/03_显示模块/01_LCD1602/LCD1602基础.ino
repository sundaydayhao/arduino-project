/*
 * 实验名称：LCD1602 显示屏基础
 * 实验目的：学习使用 I2C 接口的 LCD1602 显示文字和数据
 * 
 * 需要安装库：LiquidCrystal_I2C
 * 安装方法：Arduino IDE → 工具 → 管理库 → 搜索 "LiquidCrystal I2C"
 * 
 * 硬件连接（I2C 接口）：
 *   VCC → 5V
 *   GND → GND
 *   SDA → A4（Arduino UNO）
 *   SCL → A5（Arduino UNO）
 * 
 * I2C 地址扫描：如果不知道地址，先运行 I2C 扫描程序
 * 常见地址：0x27 或 0x3F
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// 创建 LCD 对象（地址, 列数, 行数）
LiquidCrystal_I2C lcd(0x27, 16, 2);

// 自定义字符：心形
byte heart[8] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000,
  0b00000
};

// 自定义字符：笑脸
byte smiley[8] = {
  0b00000,
  0b01010,
  0b00000,
  0b00000,
  0b10001,
  0b01110,
  0b00000,
  0b00000
};

int counter = 0;

void setup() {
  lcd.init();
  lcd.backlight();
  
  // 注册自定义字符（最多8个，编号0~7）
  lcd.createChar(0, heart);
  lcd.createChar(1, smiley);
  
  // 显示欢迎信息
  lcd.setCursor(0, 0);
  lcd.print("Arduino LCD1602");
  lcd.setCursor(0, 1);
  lcd.print("Hello World! ");
  lcd.write(0);  // 显示心形
  lcd.write(1);  // 显示笑脸
  
  delay(2000);
  lcd.clear();
  
  Serial.begin(9600);
  Serial.println("LCD1602 实验开始");
}

void loop() {
  // 第一行：显示计数器
  lcd.setCursor(0, 0);
  lcd.print("Count: ");
  lcd.print(counter);
  lcd.print("    ");  // 清除多余字符
  
  // 第二行：显示运行时间
  lcd.setCursor(0, 1);
  unsigned long seconds = millis() / 1000;
  lcd.print("Time: ");
  lcd.print(seconds / 3600);
  lcd.print(":");
  if ((seconds % 3600) / 60 < 10) lcd.print("0");
  lcd.print((seconds % 3600) / 60);
  lcd.print(":");
  if (seconds % 60 < 10) lcd.print("0");
  lcd.print(seconds % 60);
  
  counter++;
  delay(1000);
}