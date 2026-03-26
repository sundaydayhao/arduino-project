/*
 * 实验名称：光敏电阻（LDR）
 * 实验目的：读取光照强度，实现自动调光
 * 
 * 硬件连接（分压电路）：
 *   5V ──── 10kΩ ──┬──── A0
 *                  │
 *                 LDR
 *                  │
 *   GND ───────────┘
 * 
 * 原理：光照越强，LDR 阻值越小，A0 电压越低
 * 
 * 知识点：
 *   - 光敏电阻（LDR）：光照增强，阻值减小
 *   - 分压电路
 *   - 自动调光控制
 */

const int LDR_PIN = A0;
const int LED_PIN = 9;   // PWM 引脚

// 校准值（根据实际环境调整）
const int DARK_VALUE  = 900;  // 黑暗时的 ADC 值
const int LIGHT_VALUE = 100;  // 明亮时的 ADC 值

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
  Serial.println("光敏电阻实验");
  Serial.println("光照越强，LED 越暗（自动调光）");
}

void loop() {
  int ldrValue = analogRead(LDR_PIN);
  
  // 将光照值映射到 LED 亮度（光强时 LED 暗，黑暗时 LED 亮）
  int brightness = map(ldrValue, LIGHT_VALUE, DARK_VALUE, 0, 255);
  brightness = constrain(brightness, 0, 255);
  
  analogWrite(LED_PIN, brightness);
  
  // 判断光照等级
  String lightLevel;
  if (ldrValue < 200) lightLevel = "强光";
  else if (ldrValue < 400) lightLevel = "正常";
  else if (ldrValue < 700) lightLevel = "昏暗";
  else lightLevel = "黑暗";
  
  Serial.print("LDR: ");
  Serial.print(ldrValue);
  Serial.print(" | 亮度: ");
  Serial.print(brightness);
  Serial.print(" | 光照: ");
  Serial.println(lightLevel);
  
  delay(200);
}