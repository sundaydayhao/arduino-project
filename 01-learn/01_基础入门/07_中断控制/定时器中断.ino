/*
 * 实验名称：定时器中断
 * 实验目的：使用 Timer1 定时器中断，精确控制时间
 * 
 * 需要安装库：TimerOne
 * 安装方法：Arduino IDE → 工具 → 管理库 → 搜索 "TimerOne"
 * 
 * 硬件连接：
 *   LED → D13
 * 
 * 知识点：
 *   - 硬件定时器：独立于 CPU 运行，精度高
 *   - Timer1：16位定时器，可设置 1μs ~ 8.3s 周期
 *   - 定时器中断不受 delay() 影响
 */

#include <TimerOne.h>

const int LED_PIN = 13;
volatile bool ledState = false;
volatile unsigned long interruptCount = 0;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
  Serial.println("定时器中断实验");
  
  // 初始化 Timer1，设置周期为 500000 微秒（0.5秒）
  Timer1.initialize(500000);
  
  // 注册定时器中断回调函数
  Timer1.attachInterrupt(timerISR);
  
  Serial.println("定时器已启动，LED 每 0.5 秒切换一次");
}

void loop() {
  // 主循环做其他事情，不影响 LED 闪烁
  Serial.print("中断次数: ");
  Serial.print(interruptCount);
  Serial.print(" | LED: ");
  Serial.println(ledState ? "亮" : "灭");
  delay(1000);
}

// 定时器中断服务函数
void timerISR() {
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState);
  interruptCount++;
}