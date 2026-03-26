/*
 * 实验名称：8×8 点阵 LED（MAX7219 驱动）
 * 实验目的：学习点阵 LED 显示图案和滚动文字
 * 
 * 需要安装库：MD_MAX72XX 或 LedControl
 * 安装方法：Arduino IDE → 工具 → 管理库 → 搜索 "MD_MAX72XX"
 * 
 * 硬件连接：
 *   VCC → 5V
 *   GND → GND
 *   DIN → D11（MOSI）
 *   CS  → D10
 *   CLK → D13（SCK）
 */

#include <MD_MAX72xx.h>
#include <SPI.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES   1
#define CS_PIN        10

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// 自定义图案（8×8 位图）
const uint8_t HEART[] = {
  0b00000000,
  0b01100110,
  0b11111111,
  0b11111111,
  0b01111110,
  0b00111100,
  0b00011000,
  0b00000000
};

const uint8_t SMILEY[] = {
  0b00111100,
  0b01000010,
  0b10100101,
  0b10000001,
  0b10100101,
  0b10011001,
  0b01000010,
  0b00111100
};

const uint8_t ARROW[] = {
  0b00010000,
  0b00110000,
  0b01110000,
  0b11111111,
  0b11111111,
  0b01110000,
  0b00110000,
  0b00010000
};

void setup() {
  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, 5);  // 亮度 0~15
  mx.clear();
  
  Serial.begin(9600);
  Serial.println("8×8 点阵 LED 实验");
}

void loop() {
  // 显示心形
  Serial.println("显示心形");
  showPattern(HEART);
  delay(1500);
  
  // 显示笑脸
  Serial.println("显示笑脸");
  showPattern(SMILEY);
  delay(1500);
  
  // 显示箭头
  Serial.println("显示箭头");
  showPattern(ARROW);
  delay(1500);
  
  // 滚动显示数字
  Serial.println("滚动数字");
  scrollNumber();
  
  // 全亮全灭闪烁
  Serial.println("闪烁");
  for (int i = 0; i < 3; i++) {
    mx.control(MD_MAX72XX::TEST, MD_MAX72XX::ON);
    delay(300);
    mx.control(MD_MAX72XX::TEST, MD_MAX72XX::OFF);
    delay(300);
  }
}

void showPattern(const uint8_t* pattern) {
  mx.clear();
  for (int row = 0; row < 8; row++) {
    mx.setRow(0, row, pattern[row]);
  }
}

void scrollNumber() {
  // 简单的列移位动画
  for (int shift = 0; shift < 8; shift++) {
    mx.clear();
    for (int col = 0; col < 8 - shift; col++) {
      mx.setColumn(0, col + shift, 0xFF);
    }
    delay(100);
  }
  mx.clear();
}