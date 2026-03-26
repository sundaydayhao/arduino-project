/*
 * 实验名称：声音传感器
 * 实验目的：检测声音，实现声控灯
 * 
 * 硬件连接：
 *   VCC → 5V
 *   GND → GND
 *   DO  → D2（数字输出，超过阈值输出 HIGH）
 *   AO  → A0（模拟输出，输出声音强度）
 * 
 * 模块上的旋钮：调节触发灵敏度阈值
 * 
 * 知识点：
 *   - 声音传感器原理：麦克风 + 比较器
 *   - 数字输出 vs 模拟输出
 *   - 声控灯实现
 */

const int SOUND_DO = 2;   // 数字输出
const int SOUND_AO = A0;  // 模拟输出
const int LED_PIN  = 13;

// 声控灯参数
const unsigned long LIGHT_DURATION = 5000;  // 亮灯持续时间 5 秒
unsigned long lastSoundTime = 0;
bool lightOn = false;

void setup() {
  pinMode(SOUND_DO, INPUT);
  pinMode(LED_PIN, OUTPUT);
  
  Serial.begin(9600);
  Serial.println("声音传感器实验");
  Serial.println("拍手或发出声音触发 LED");
}

void loop() {
  // 读取数字输出（超过阈值为 HIGH）
  int soundDetected = digitalRead(SOUND_DO);
  
  // 读取模拟输出（声音强度）
  int soundLevel = analogRead(SOUND_AO);
  
  // 声控灯逻辑
  if (soundDetected == HIGH) {
    lastSoundTime = millis();
    if (!lightOn) {
      lightOn = true;
      digitalWrite(LED_PIN, HIGH);
      Serial.print("检测到声音！强度: ");
      Serial.println(soundLevel);
    }
  }
  
  // 超过持续时间后关灯
  if (lightOn && (millis() - lastSoundTime > LIGHT_DURATION)) {
    lightOn = false;
    digitalWrite(LED_PIN, LOW);
    Serial.println("声音消失，关灯");
  }
  
  // 显示声音波形（串口绘图仪）
  Serial.println(soundLevel);
  
  delay(10);
}