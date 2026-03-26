/*
 * 实验名称：电位器控制舵机
 * 实验目的：用电位器实时控制舵机角度
 * 
 * 硬件连接：
 *   电位器中间脚 → A0
 *   电位器两端 → 5V 和 GND
 *   舵机信号线 → D9
 *   舵机 VCC → 5V（建议外部供电）
 *   舵机 GND → GND
 */

#include <Servo.h>

Servo myServo;
const int POT_PIN   = A0;
const int SERVO_PIN = 9;

int lastAngle = -1;  // 记录上次角度，避免重复写入

void setup() {
  myServo.attach(SERVO_PIN);
  Serial.begin(9600);
  Serial.println("电位器控制舵机实验");
  Serial.println("旋转电位器控制舵机角度");
}

void loop() {
  // 读取电位器值
  int potValue = analogRead(POT_PIN);
  
  // 映射到 0~180 度
  int angle = map(potValue, 0, 1023, 0, 180);
  
  // 只在角度变化时更新（减少抖动）
  if (abs(angle - lastAngle) > 1) {
    myServo.write(angle);
    lastAngle = angle;
    
    Serial.print("电位器: ");
    Serial.print(potValue);
    Serial.print(" → 角度: ");
    Serial.print(angle);
    Serial.println("°");
  }
  
  delay(20);
}