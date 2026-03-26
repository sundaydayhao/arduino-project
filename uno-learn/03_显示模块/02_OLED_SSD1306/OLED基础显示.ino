/*
 * 实验名称：OLED SSD1306 显示屏
 * 实验目的：学习使用 OLED 显示文字、图形和动画
 * 
 * 需要安装库：
 *   - Adafruit SSD1306
 *   - Adafruit GFX Library
 * 
 * 硬件连接（I2C 接口，0.96" OLED）：
 *   VCC → 3.3V 或 5V
 *   GND → GND
 *   SDA → A4
 *   SCL → A5
 * 
 * 分辨率：128×64 像素
 * I2C 地址：0x3C 或 0x3D
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1  // 无复位引脚
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(9600);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("OLED 初始化失败！检查接线和地址");
    while (true);
  }
  
  // 显示 Adafruit Logo（内置）
  display.display();
  delay(2000);
  display.clearDisplay();
  
  // 显示欢迎文字
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println("Arduino");
  display.setTextSize(1);
  display.setCursor(20, 35);
  display.println("OLED SSD1306");
  display.setCursor(25, 50);
  display.println("128 x 64 px");
  display.display();
  delay(3000);
}

void loop() {
  // 演示1：文字显示
  demoText();
  delay(2000);
  
  // 演示2：图形绘制
  demoShapes();
  delay(2000);
  
  // 演示3：动画
  demoAnimation();
}

void demoText() {
  display.clearDisplay();
  
  // 不同大小的文字
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Size 1: Hello!");
  
  display.setTextSize(2);
  display.setCursor(0, 16);
  display.println("Size 2");
  
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print("Time: ");
  display.print(millis() / 1000);
  display.println("s");
  
  display.display();
}

void demoShapes() {
  display.clearDisplay();
  
  // 矩形
  display.drawRect(0, 0, 30, 20, SSD1306_WHITE);
  display.fillRect(35, 0, 30, 20, SSD1306_WHITE);
  
  // 圆形
  display.drawCircle(10, 40, 10, SSD1306_WHITE);
  display.fillCircle(40, 40, 10, SSD1306_WHITE);
  
  // 三角形
  display.drawTriangle(70, 0, 60, 20, 80, 20, SSD1306_WHITE);
  display.fillTriangle(90, 0, 80, 20, 100, 20, SSD1306_WHITE);
  
  // 直线
  display.drawLine(0, 55, 127, 55, SSD1306_WHITE);
  
  display.display();
}

void demoAnimation() {
  // 弹跳小球动画
  int x = 10, y = 10;
  int dx = 3, dy = 2;
  int radius = 5;
  
  for (int frame = 0; frame < 100; frame++) {
    display.clearDisplay();
    
    // 绘制小球
    display.fillCircle(x, y, radius, SSD1306_WHITE);
    
    // 绘制边框
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    
    // 显示帧数
    display.setTextSize(1);
    display.setCursor(2, 2);
    display.print("Frame: ");
    display.print(frame);
    
    display.display();
    
    // 更新位置
    x += dx;
    y += dy;
    
    // 碰壁反弹
    if (x <= radius || x >= SCREEN_WIDTH - radius) dx = -dx;
    if (y <= radius || y >= SCREEN_HEIGHT - radius) dy = -dy;
    
    delay(20);
  }
}