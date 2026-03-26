/*
 * 实验名称：MQ-2 气体传感器
 * 实验目的：检测可燃气体和烟雾，实现气体报警
 * 
 * 硬件连接：
 *   VCC → 5V
 *   GND → GND
 *   DO  → D2（数字输出，超过阈值输出 LOW）
 *   AO  → A0（模拟输出，气体浓度）
 * 
 * ⚠️ 注意：
 *   - 传感器上电后需要预热 20~30 秒
 *   - 传感器工作时会发热，属正常现象
 *   - 不要在传感器附近使用明火！
 * 
 * MQ-2 可检测：
 *   液化气、丁烷、丙烷、甲烷、酒精、氢气、烟雾
 */

const int MQ2_DO  = 2;
const int MQ2_AO  = A0;
const int LED_RED = 13;
const int BUZZER  = 8;

// 报警阈值（根据实际环境校准）
const int ALARM_THRESHOLD = 400;

void setup() {
  pinMode(MQ2_DO, INPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  
  Serial.begin(9600);
  Serial.println("MQ-2 气体传感器实验");
  Serial.println("⚠️  传感器预热中，请等待 30 秒...");
  
  // 预热等待
  for (int i = 30; i > 0; i--) {
    Serial.print(i);
    Serial.print("s ");
    delay(1000);
  }
  Serial.println("\n传感器就绪！");
  Serial.println("气体值 | 状态");
}

void loop() {
  int gasValue = analogRead(MQ2_AO);
  int doValue  = digitalRead(MQ2_DO);
  
  Serial.print("气体浓度: ");
  Serial.print(gasValue);
  Serial.print(" | DO: ");
  Serial.print(doValue == LOW ? "报警" : "正常");
  Serial.print(" | 状态: ");
  
  if (gasValue > ALARM_THRESHOLD || doValue == LOW) {
    // 报警
    digitalWrite(LED_RED, HIGH);
    tone(BUZZER, 2000, 500);
    Serial.println("⚠️  危险！检测到气体！");
  } else if (gasValue > ALARM_THRESHOLD * 0.7) {
    // 预警
    digitalWrite(LED_RED, HIGH);
    delay(500);
    digitalWrite(LED_RED, LOW);
    Serial.println("⚠️  注意：气体浓度偏高");
  } else {
    // 正常
    digitalWrite(LED_RED, LOW);
    noTone(BUZZER);
    Serial.println("正常");
  }
  
  delay(1000);
}