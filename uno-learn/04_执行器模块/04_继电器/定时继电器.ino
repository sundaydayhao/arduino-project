/*
 * 实验名称：定时继电器
 * 实验目的：实现定时开关功能
 * 
 * 功能：
 *   - 每隔设定时间自动切换继电器状态
 *   - 可通过串口设置开/关时间
 * 
 * 硬件连接：
 *   继电器 IN → D8
 */

const int RELAY_PIN = 8;
const int RELAY_ON  = LOW;
const int RELAY_OFF = HIGH;

// 定时参数（毫秒）
unsigned long onDuration  = 5000;   // 开启持续时间 5 秒
unsigned long offDuration = 10000;  // 关闭持续时间 10 秒

bool relayState = false;
unsigned long lastSwitchTime = 0;

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);
  
  Serial.begin(9600);
  Serial.println("定时继电器实验");
  Serial.println("命令: SET ON <秒> | SET OFF <秒> | STATUS");
  Serial.print("当前设置: 开 ");
  Serial.print(onDuration / 1000);
  Serial.print("s / 关 ");
  Serial.print(offDuration / 1000);
  Serial.println("s");
}

void loop() {
  unsigned long now = millis();
  
  // 定时切换
  if (relayState && (now - lastSwitchTime >= onDuration)) {
    relayState = false;
    digitalWrite(RELAY_PIN, RELAY_OFF);
    lastSwitchTime = now;
    Serial.println("定时关闭");
  }
  else if (!relayState && (now - lastSwitchTime >= offDuration)) {
    relayState = true;
    digitalWrite(RELAY_PIN, RELAY_ON);
    lastSwitchTime = now;
    Serial.println("定时开启");
  }
  
  // 串口命令
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    if (cmd.startsWith("SET ON ")) {
      onDuration = cmd.substring(7).toInt() * 1000UL;
      Serial.print("开启时间设为: ");
      Serial.print(onDuration / 1000);
      Serial.println("s");
    }
    else if (cmd.startsWith("SET OFF ")) {
      offDuration = cmd.substring(8).toInt() * 1000UL;
      Serial.print("关闭时间设为: ");
      Serial.print(offDuration / 1000);
      Serial.println("s");
    }
    else if (cmd == "STATUS") {
      Serial.print("继电器: ");
      Serial.print(relayState ? "ON" : "OFF");
      Serial.print(" | 开 ");
      Serial.print(onDuration / 1000);
      Serial.print("s / 关 ");
      Serial.print(offDuration / 1000);
      Serial.println("s");
    }
  }
}