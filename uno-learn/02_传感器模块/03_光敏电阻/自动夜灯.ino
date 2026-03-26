/*
 * 实验名称：自动夜灯
 * 实验目的：天黑自动开灯，天亮自动关灯
 * 
 * 硬件连接：
 *   LDR 分压电路 → A0
 *   LED → D9（PWM）
 *   继电器（可选，控制真实灯泡）→ D8
 * 
 * 功能：
 *   - 光照低于阈值时自动开灯
 *   - 光照高于阈值时自动关灯
 *   - 带迟滞（防止频繁切换）
 */

const int LDR_PIN   = A0;
const int LED_PIN   = 9;
const int RELAY_PIN = 8;

// 阈值（带迟滞，防止抖动）
const int THRESHOLD_ON  = 600;  // 低于此值开灯
const int THRESHOLD_OFF = 400;  // 高于此值关灯

bool lightOn = false;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("自动夜灯系统");
  Serial.print("开灯阈值: ");
  Serial.print(THRESHOLD_ON);
  Serial.print(" | 关灯阈值: ");
  Serial.println(THRESHOLD_OFF);
}

void loop() {
  // 多次采样取平均，减少噪声
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(LDR_PIN);
    delay(10);
  }
  int ldrValue = sum / 10;
  
  // 迟滞控制
  if (!lightOn && ldrValue > THRESHOLD_ON) {
    // 变暗，开灯
    lightOn = true;
    digitalWrite(RELAY_PIN, HIGH);
    Serial.println("天黑了 → 开灯");
  }
  else if (lightOn && ldrValue < THRESHOLD_OFF) {
    // 变亮，关灯
    lightOn = false;
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("天亮了 → 关灯");
  }
  
  // LED 模拟灯光（亮度与黑暗程度成正比）
  if (lightOn) {
    int brightness = map(ldrValue, THRESHOLD_OFF, 1023, 50, 255);
    brightness = constrain(brightness, 50, 255);
    analogWrite(LED_PIN, brightness);
  } else {
    analogWrite(LED_PIN, 0);
  }
  
  Serial.print("LDR: ");
  Serial.print(ldrValue);
  Serial.print(" | 灯: ");
  Serial.println(lightOn ? "开" : "关");
  
  delay(500);
}