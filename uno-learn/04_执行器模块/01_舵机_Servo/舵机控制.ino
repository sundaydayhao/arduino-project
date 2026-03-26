/*
 * 实验名称：舵机控制
 * 实验目的：学习使用 Servo 库控制舵机角度
 * 
 * 硬件连接：
 *   舵机红线（VCC）→ 5V（建议外部供电）
 *   舵机棕线（GND）→ GND
 *   舵机橙线（信号）→ D9
 * 
 * ⚠️ 注意：舵机电流较大，建议使用外部 5V 电源供电
 *    多个舵机必须使用外部电源！
 * 
 * 知识点：
 *   - PWM 控制舵机：50Hz，脉宽 0.5ms~2.5ms
 *   - SG90：0°~180°，扭矩 1.8kg·cm
 *   - Servo 库简化了舵机控制
 */

#include <Servo.h>

Servo myServo;
const int SERVO_PIN = 9;

void setup() {
  myServo.attach(SERVO_PIN);  // 连接舵机到引脚9
  Serial.begin(9600);
  Serial.println("舵机控制实验");
  Serial.println("输入角度(0~180)控制舵机，或按 's' 扫描");
  
  // 初始位置：90度（中间）
  myServo.write(90);
  delay(1000);
}

void loop() {
  // 检查串口输入
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input == "s" || input == "S") {
      // 扫描模式
      sweepServo();
    } else {
      int angle = input.toInt();
      if (angle >= 0 && angle <= 180) {
        myServo.write(angle);
        Serial.print("舵机转到: ");
        Serial.print(angle);
        Serial.println("°");
      } else {
        Serial.println("角度范围: 0~180");
      }
    }
  }
}

// 舵机扫描（0° → 180° → 0°）
void sweepServo() {
  Serial.println("开始扫描...");
  
  // 0° → 180°
  for (int angle = 0; angle <= 180; angle++) {
    myServo.write(angle);
    delay(15);
  }
  
  delay(500);
  
  // 180° → 0°
  for (int angle = 180; angle >= 0; angle--) {
    myServo.write(angle);
    delay(15);
  }
  
  Serial.println("扫描完成");
}