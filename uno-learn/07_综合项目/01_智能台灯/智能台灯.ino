/*
 * 综合项目：智能台灯
 * 功能：
 *   1. 光控：环境暗时自动开灯
 *   2. 人感：有人时开灯，无人延时关灯
 *   3. 手动：按键切换开关
 *   4. 调光：电位器调节亮度
 *   5. 串口：显示状态信息
 * 
 * 硬件：
 *   LDR → A0（光敏电阻分压）
 *   PIR → D2（人体感应）
 *   按键 → D3
 *   电位器 → A1
 *   LED（台灯）→ D9（PWM）
 */

const int LDR_PIN    = A0;
const int PIR_PIN    = 2;
const int BTN_PIN    = 3;
const int POT_PIN    = A1;
const int LAMP_PIN   = 9;

// 参数设置
const int DARK_THRESHOLD  = 600;   // 光照阈值（低于此值认为暗）
const unsigned long AUTO_OFF_TIME = 30000;  // 无人后30秒关灯

// 状态变量
bool lampOn = false;
bool manualMode = false;
int brightness = 200;
unsigned long lastMotionTime = 0;
int lastBtnState = HIGH;
unsigned long lastDebounce = 0;

void setup() {
  pinMode(PIR_PIN, INPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(LAMP_PIN, OUTPUT);
  
  Serial.begin(9600);
  Serial.println("=== 智能台灯系统 ===");
  Serial.println("等待PIR传感器稳定(30s)...");
  delay(30000);
  Serial.println("系统就绪！");
}

void loop() {
  // 读取传感器
  int ldrValue  = analogRead(LDR_PIN);
  int potValue  = analogRead(POT_PIN);
  int pirState  = digitalRead(PIR_PIN);
  bool isDark   = (ldrValue > DARK_THRESHOLD);
  
  // 电位器调节亮度
  brightness = map(potValue, 0, 1023, 50, 255);
  
  // 按键切换手动/自动模式
  int btnReading = digitalRead(BTN_PIN);
  if (btnReading != lastBtnState) lastDebounce = millis();
  if ((millis() - lastDebounce) > 50 && btnReading == LOW && lastBtnState == HIGH) {
    manualMode = !manualMode;
    if (manualMode) lampOn = !lampOn;
    Serial.print("切换到");
    Serial.println(manualMode ? "手动模式" : "自动模式");
  }
  lastBtnState = btnReading;
  
  // 自动模式逻辑
  if (!manualMode) {
    if (pirState == HIGH) {
      lastMotionTime = millis();
      if (isDark) lampOn = true;
    }
    if (lampOn && (millis() - lastMotionTime > AUTO_OFF_TIME)) {
      lampOn = false;
      Serial.println("无人，自动关灯");
    }
    if (!isDark) lampOn = false;  // 环境亮时关灯
  }
  
  // 控制台灯
  analogWrite(LAMP_PIN, lampOn ? brightness : 0);
  
  // 串口状态输出（每2秒）
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 2000) {
    lastPrint = millis();
    Serial.print("光照:");
    Serial.print(ldrValue);
    Serial.print(isDark ? "(暗)" : "(亮)");
    Serial.print(" PIR:");
    Serial.print(pirState ? "有人" : "无人");
    Serial.print(" 亮度:");
    Serial.print(brightness);
    Serial.print(" 台灯:");
    Serial.print(lampOn ? "开" : "关");
    Serial.print(" 模式:");
    Serial.println(manualMode ? "手动" : "自动");
  }
  
  delay(100);
}