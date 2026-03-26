/*
 * 实验名称：4位7段数码管
 * 实验目的：学习数码管的驱动原理，显示数字和时间
 * 
 * 需要安装库：TM1637（如果使用 TM1637 驱动的数码管）
 * 安装方法：Arduino IDE → 工具 → 管理库 → 搜索 "TM1637"
 * 
 * 硬件连接（TM1637 4位数码管）：
 *   CLK → D2
 *   DIO → D3
 *   VCC → 5V
 *   GND → GND
 * 
 * 知识点：
 *   - 7段数码管编码
 *   - 动态扫描原理
 *   - TM1637 驱动芯片
 */

#include <TM1637Display.h>

const int CLK_PIN = 2;
const int DIO_PIN = 3;

TM1637Display display(CLK_PIN, DIO_PIN);

// 7段编码（用于直接控制段）
const uint8_t SEG_DONE[] = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_C | SEG_E | SEG_G,                            // n
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G             // E
};

void setup() {
  display.setBrightness(5);  // 亮度 0~7
  
  // 显示 "DONE"
  display.setSegments(SEG_DONE);
  delay(2000);
  
  Serial.begin(9600);
  Serial.println("4位数码管实验");
}

void loop() {
  // 显示计数 0~9999
  for (int i = 0; i <= 9999; i++) {
    display.showNumberDec(i, true);  // true = 显示前导零
    delay(10);
  }
  
  // 显示时间格式（带冒号）
  Serial.println("显示时间格式...");
  for (int seconds = 0; seconds < 60; seconds++) {
    int minutes = seconds / 60;
    int secs = seconds % 60;
    
    // 格式：MM:SS
    int timeValue = minutes * 100 + secs;
    display.showNumberDecEx(timeValue, 0b01000000, true);  // 0b01000000 = 冒号
    delay(1000);
  }
}