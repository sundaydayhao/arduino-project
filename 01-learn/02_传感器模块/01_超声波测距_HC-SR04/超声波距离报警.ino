/*
 * 实验名称：超声波距离报警
 * 实验目的：根据距离控制 LED 和蜂鸣器，实现接近报警
 * 
 * 硬件连接：
 *   HC-SR04: VCC→5V, GND→GND, Trig→D9, Echo→D10
 *   蜂鸣器 → D8
 *   绿色 LED → D5
 *   黄色 LED → D6
 *   红色 LED → D7
 * 
 * 距离区间：
 *   > 50cm：绿灯，无声
 *   20~50cm：黄灯，慢速蜂鸣
 *   < 20cm：红灯，快速蜂鸣
 */

const int TRIG_PIN   = 9;
const int ECHO_PIN   = 10;
const int BUZZER_PIN = 8;
const int LED_GREEN  = 5;
const int LED_YELLOW = 6;
const int LED_RED    = 7;

const float DIST_FAR  = 50.0;  // 安全距离
const float DIST_NEAR = 20.0;  // 危险距离

unsigned long lastBeepTime = 0;
bool beepState = false;

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  
  Serial.begin(9600);
  Serial.println("超声波距离报警系统");
}

void loop() {
  float distance = measureDistance();
  unsigned long now = millis();
  
  // 关闭所有 LED
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);
  
  if (distance < 0) {
    Serial.println("超出范围");
    noTone(BUZZER_PIN);
    return;
  }
  
  Serial.print("距离: ");
  Serial.print(distance, 1);
  Serial.print(" cm → ");
  
  if (distance > DIST_FAR) {
    // 安全区域
    digitalWrite(LED_GREEN, HIGH);
    noTone(BUZZER_PIN);
    Serial.println("安全 ✓");
  }
  else if (distance > DIST_NEAR) {
    // 警告区域
    digitalWrite(LED_YELLOW, HIGH);
    // 每 500ms 蜂鸣一次
    if (now - lastBeepTime > 500) {
      lastBeepTime = now;
      tone(BUZZER_PIN, 1000, 100);
    }
    Serial.println("警告 ⚠");
  }
  else {
    // 危险区域
    digitalWrite(LED_RED, HIGH);
    // 每 200ms 蜂鸣一次
    if (now - lastBeepTime > 200) {
      lastBeepTime = now;
      tone(BUZZER_PIN, 2000, 100);
    }
    Serial.println("危险 ✗");
  }
  
  delay(100);
}

float measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;
  
  float distance = duration * 0.034 / 2.0;
  return (distance >= 2 && distance <= 400) ? distance : -1;
}