/*
 * 实验名称：继电器控制
 * 实验目的：学习使用继电器控制高压/大电流设备
 * 
 * 硬件连接：
 *   继电器模块：
 *     VCC → 5V
 *     GND → GND
 *     IN  → D8（低电平触发）或 D8（高电平触发，看模块类型）
 * 
 * ⚠️ 安全警告：
 *   - 继电器可以控制 220V 交流电，操作时务必注意安全！
 *   - 高压接线必须由专业人员操作
 *   - 实验时建议只控制低压设备（如 LED 灯带、小风扇）
 * 
 * 知识点：
 *   - 继电器：电磁开关，用小电流控制大电流
 *   - 常开（NO）：默认断开，线圈通电后闭合
 *   - 常闭（NC）：默认闭合，线圈通电后断开
 */

const int RELAY_PIN = 8;
const int BUTTON_PIN = 2;

// 继电器触发方式（根据模块类型修改）
// 低电平触发：RELAY_ON = LOW, RELAY_OFF = HIGH
// 高电平触发：RELAY_ON = HIGH, RELAY_OFF = LOW
const int RELAY_ON  = LOW;
const int RELAY_OFF = HIGH;

bool relayState = false;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // 初始关闭继电器
  digitalWrite(RELAY_PIN, RELAY_OFF);
  
  Serial.begin(9600);
  Serial.println("继电器控制实验");
  Serial.println("按键切换继电器状态");
  Serial.println("串口输入 'on'/'off' 控制");
}

void loop() {
  // 按键控制
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > 50) {
    if (reading == LOW && lastButtonState == HIGH) {
      toggleRelay();
    }
  }
  lastButtonState = reading;
  
  // 串口控制
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();
    
    if (cmd == "on") {
      setRelay(true);
    } else if (cmd == "off") {
      setRelay(false);
    } else if (cmd == "toggle") {
      toggleRelay();
    } else if (cmd == "status") {
      Serial.print("继电器状态: ");
      Serial.println(relayState ? "ON（闭合）" : "OFF（断开）");
    }
  }
}

void setRelay(bool state) {
  relayState = state;
  digitalWrite(RELAY_PIN, state ? RELAY_ON : RELAY_OFF);
  Serial.print("继电器: ");
  Serial.println(state ? "ON（闭合）" : "OFF（断开）");
}

void toggleRelay() {
  setRelay(!relayState);
}