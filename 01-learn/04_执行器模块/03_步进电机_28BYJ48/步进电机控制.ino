/*
 * 实验名称：28BYJ-48 步进电机控制
 * 实验目的：学习步进电机的驱动原理和精确位置控制
 * 
 * 需要安装库：Stepper（Arduino 内置）
 * 
 * 硬件连接（ULN2003 驱动板）：
 *   IN1 → D8
 *   IN2 → D9
 *   IN3 → D10
 *   IN4 → D11
 *   VCC → 5V（建议外部供电）
 *   GND → GND
 * 
 * 28BYJ-48 规格：
 *   步距角：5.625°/64（减速比1:64）
 *   每圈步数：64 × 64 = 4096 步（半步模式）
 *   工作电压：5V
 */

#include <Stepper.h>

// 28BYJ-48 每圈步数（半步模式）
const int STEPS_PER_REV = 2048;

// 创建步进电机对象（步数, IN1, IN3, IN2, IN4）
// 注意引脚顺序！
Stepper myStepper(STEPS_PER_REV, 8, 10, 9, 11);

void setup() {
  // 设置转速（RPM）
  myStepper.setSpeed(10);  // 10 RPM
  
  Serial.begin(9600);
  Serial.println("28BYJ-48 步进电机实验");
}

void loop() {
  // 顺时针转一圈
  Serial.println("顺时针转一圈...");
  myStepper.step(STEPS_PER_REV);
  delay(1000);
  
  // 逆时针转一圈
  Serial.println("逆时针转一圈...");
  myStepper.step(-STEPS_PER_REV);
  delay(1000);
  
  // 转 90 度
  Serial.println("顺时针转 90°...");
  myStepper.step(STEPS_PER_REV / 4);
  delay(1000);
  
  // 转 180 度
  Serial.println("逆时针转 180°...");
  myStepper.step(-STEPS_PER_REV / 2);
  delay(1000);
  
  // 回到原位
  Serial.println("回到原位...");
  myStepper.step(STEPS_PER_REV / 4);
  delay(2000);
}