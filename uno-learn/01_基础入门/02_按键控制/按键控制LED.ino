/*
 * 实验名称：按键控制 LED
 * 实验目的：学习数字输入，用按键控制 LED 亮灭
 * 
 * 硬件连接：
 *   按键一端 → Arduino D2
 *   按键另一端 → GND
 *   LED 正极 → 220Ω 电阻 → Arduino D13
 *   LED 负极 → GND
 * 
 * 知识点：
 *   - INPUT_PULLUP 内部上拉电阻
 *   - digitalRead() 数字输入
 *   - 按键消抖
 */

const int BUTTON_PIN = 2;   // 按键引脚
const int LED_PIN = 13;     // LED 引脚

void setup() {
  // 使用内部上拉电阻，按下时读到 LOW
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  
  Serial.begin(9600);
  Serial.println("按键控制 LED 实验开始！");
  Serial.println("按下按键点亮 LED，松开熄灭");
}

void loop() {
  // 读取按键状态（使用上拉电阻，按下为 LOW）
  int buttonState = digitalRead(BUTTON_PIN);
  
  if (buttonState == LOW) {
    // 按键按下
    digitalWrite(LED_PIN, HIGH);
    Serial.println("按键按下 → LED 亮");
  } else {
    // 按键松开
    digitalWrite(LED_PIN, LOW);
  }
}