/*
 * 实验名称：PIR 人体红外感应
 * 实验目的：检测人体运动，实现感应灯
 * 
 * 硬件连接：
 *   VCC → 5V
 *   OUT → D2
 *   GND → GND
 * 
 * PIR 模块调节：
 *   - 灵敏度旋钮：调节检测距离（3~7m）
 *   - 延时旋钮：调节触发后保持时间（3s~5min）
 *   - 跳线帽：H=重复触发，L=单次触发
 * 
 * 工作原理：
 *   检测人体发出的红外辐射变化（热释电效应）
 */

const int PIR_PIN = 2;
const int LED_PIN = 13;

bool motionDetected = false;
unsigned long motionTime = 0;
const unsigned long LIGHT_DURATION = 10000;  // 灯亮持续时间 10 秒

void setup() {
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  
  Serial.begin(9600);
  Serial.println("PIR 人体感应实验");
  Serial.println("等待传感器稳定（约30秒）...");
  
  // PIR 传感器需要约 30 秒预热稳定
  for (int i = 30; i > 0; i--) {
    Serial.print(i);
    Serial.print("s ");
    delay(1000);
  }
  Serial.println("\n传感器就绪！");
}

void loop() {
  int pirState = digitalRead(PIR_PIN);
  
  if (pirState == HIGH) {
    if (!motionDetected) {
      motionDetected = true;
      Serial.println("检测到人体运动！");
    }
    motionTime = millis();  // 更新最后检测时间
    digitalWrite(LED_PIN, HIGH);
  } else {
    // 超过持续时间后关灯
    if (motionDetected && (millis() - motionTime > LIGHT_DURATION)) {
      motionDetected = false;
      digitalWrite(LED_PIN, LOW);
      Serial.println("无人，关灯");
    }
  }
  
  delay(100);
}