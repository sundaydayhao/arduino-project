/*
 * 实验名称：PWM 调光
 * 实验目的：学习 PWM（脉冲宽度调制）控制 LED 亮度
 * 
 * 硬件连接：
 *   LED 正极 → 220Ω 电阻 → Arduino D9（PWM引脚）
 *   LED 负极 → GND
 * 
 * 知识点：
 *   - PWM 原理：通过快速开关来模拟模拟信号
 *   - analogWrite(pin, value)：value 范围 0~255
 *   - Arduino UNO PWM 引脚：3, 5, 6, 9, 10, 11（带~标记）
 */

const int LED_PIN = 9;  // 必须是 PWM 引脚

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
  Serial.println("PWM 调光实验开始！");
}

void loop() {
  // 从暗到亮（渐亮）
  Serial.println("渐亮...");
  for (int brightness = 0; brightness <= 255; brightness++) {
    analogWrite(LED_PIN, brightness);
    delay(10);
  }
  
  delay(500);
  
  // 从亮到暗（渐灭）
  Serial.println("渐灭...");
  for (int brightness = 255; brightness >= 0; brightness--) {
    analogWrite(LED_PIN, brightness);
    delay(10);
  }
  
  delay(500);
  
  // 呼吸灯效果（使用正弦波）
  Serial.println("呼吸灯效果...");
  for (int i = 0; i < 360; i++) {
    // 使用正弦函数产生平滑的呼吸效果
    float rad = i * PI / 180.0;
    int brightness = (int)(127.5 * (1 + sin(rad)));
    analogWrite(LED_PIN, brightness);
    delay(8);
  }
}