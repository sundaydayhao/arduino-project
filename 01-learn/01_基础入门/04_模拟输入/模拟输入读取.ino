/*
 * 实验名称：模拟输入读取
 * 实验目的：学习读取各种模拟传感器的值
 * 
 * 硬件连接：
 *   电位器中间脚 → A0
 *   光敏电阻 + 10kΩ分压 → A1
 *   热敏电阻 + 10kΩ分压 → A2
 * 
 * 知识点：
 *   - ADC（模数转换）：10位精度，0~1023
 *   - 参考电压：默认 5V
 *   - 分压电路原理
 */

const int POT_PIN = A0;    // 电位器
const int LDR_PIN = A1;    // 光敏电阻
const int TEMP_PIN = A2;   // 热敏电阻（NTC）

void setup() {
  Serial.begin(9600);
  Serial.println("模拟输入读取实验");
  Serial.println("格式: 电位器 | 光照 | 温度(估算)");
  Serial.println("-----------------------------------");
}

void loop() {
  // 读取电位器
  int potValue = analogRead(POT_PIN);
  float potVoltage = potValue * (5.0 / 1023.0);
  
  // 读取光敏电阻（值越大越暗）
  int ldrValue = analogRead(LDR_PIN);
  int lightPercent = map(ldrValue, 0, 1023, 100, 0);
  
  // 读取热敏电阻并估算温度（NTC 10kΩ @ 25°C）
  int tempRaw = analogRead(TEMP_PIN);
  float voltage = tempRaw * (5.0 / 1023.0);
  float resistance = (5.0 - voltage) / voltage * 10000.0;  // 分压电路
  // Steinhart-Hart 简化公式
  float tempK = 1.0 / (log(resistance / 10000.0) / 3950.0 + 1.0 / 298.15);
  float tempC = tempK - 273.15;
  
  // 输出结果
  Serial.print("电位器: ");
  Serial.print(potValue);
  Serial.print(" (");
  Serial.print(potVoltage, 2);
  Serial.print("V) | 光照: ");
  Serial.print(lightPercent);
  Serial.print("% | 温度: ");
  Serial.print(tempC, 1);
  Serial.println("°C");
  
  delay(500);
}