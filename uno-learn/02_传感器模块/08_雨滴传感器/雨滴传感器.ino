/*
 * 实验名称：雨滴传感器
 * 实验目的：检测雨水，实现自动关窗提醒
 * 
 * 硬件连接：
 *   VCC → 5V
 *   GND → GND
 *   DO  → D2（数字输出，有雨输出 LOW）
 *   AO  → A0（模拟输出，雨量大小）
 * 
 * 知识点：
 *   - 雨滴传感器原理：水导电，改变电阻
 *   - 模拟值越小，雨量越大
 */

const int RAIN_DO  = 2;
const int RAIN_AO  = A0;
const int LED_PIN  = 13;
const int BUZZER   = 8;

void setup() {
  pinMode(RAIN_DO, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  
  Serial.begin(9600);
  Serial.println("雨滴传感器实验");
}

void loop() {
  int rainAO = analogRead(RAIN_AO);
  int rainDO = digitalRead(RAIN_DO);
  
  // 模拟值越小，雨量越大
  int rainPercent = map(rainAO, 1023, 0, 0, 100);
  rainPercent = constrain(rainPercent, 0, 100);
  
  String rainLevel;
  if (rainAO > 800) {
    rainLevel = "无雨";
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER);
  } else if (rainAO > 500) {
    rainLevel = "小雨";
    digitalWrite(LED_PIN, HIGH);
    noTone(BUZZER);
  } else if (rainAO > 200) {
    rainLevel = "中雨";
    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER, 1000, 200);
  } else {
    rainLevel = "大雨";
    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER, 2000, 200);
  }
  
  Serial.print("AO: ");
  Serial.print(rainAO);
  Serial.print(" | 雨量: ");
  Serial.print(rainPercent);
  Serial.print("% | 等级: ");
  Serial.print(rainLevel);
  Serial.print(" | DO: ");
  Serial.println(rainDO == LOW ? "有雨" : "无雨");
  
  delay(500);
}