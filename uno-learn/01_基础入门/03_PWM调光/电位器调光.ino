/*
 * 实验名称：电位器控制 LED 亮度
 * 实验目的：读取模拟输入，映射到 PWM 输出，实现手动调光
 * 
 * 硬件连接：
 *   电位器中间脚 → Arduino A0
 *   电位器一端 → 5V
 *   电位器另一端 → GND
 *   LED 正极 → 220Ω 电阻 → Arduino D9
 *   LED 负极 → GND
 * 
 * 知识点：
 *   - analogRead()：读取模拟值（0~1023）
 *   - map()：数值映射函数
 *   - analogWrite()：PWM 输出（0~255）
 */

const int POT_PIN = A0;   // 电位器引脚
const int LED_PIN = 9;    // LED PWM 引脚

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
  Serial.println("电位器调光实验开始！");
  Serial.println("旋转电位器调节 LED 亮度");
}

void loop() {
  // 读取电位器值（0~1023）
  int potValue = analogRead(POT_PIN);
  
  // 将 0~1023 映射到 0~255
  int brightness = map(potValue, 0, 1023, 0, 255);
  
  // 设置 LED 亮度
  analogWrite(LED_PIN, brightness);
  
  // 串口输出调试信息
  Serial.print("电位器: ");
  Serial.print(potValue);
  Serial.print(" → 亮度: ");
  Serial.print(brightness);
  Serial.print(" (");
  Serial.print(map(brightness, 0, 255, 0, 100));
  Serial.println("%)");
  
  delay(100);
}