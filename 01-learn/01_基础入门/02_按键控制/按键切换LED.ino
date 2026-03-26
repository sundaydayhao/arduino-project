/*
 * 实验名称：按键切换 LED（带消抖）
 * 实验目的：每次按下按键，LED 状态翻转；学习软件消抖技术
 * 
 * 硬件连接：
 *   按键一端 → Arduino D2
 *   按键另一端 → GND
 *   LED 正极 → 220Ω 电阻 → Arduino D13
 *   LED 负极 → GND
 * 
 * 知识点：
 *   - 软件消抖（debounce）
 *   - 边沿检测（上升沿/下降沿）
 *   - 状态机思想
 */

const int BUTTON_PIN = 2;
const int LED_PIN = 13;
const unsigned long DEBOUNCE_DELAY = 50;  // 消抖时间 50ms

int ledState = LOW;           // LED 当前状态
int lastButtonState = HIGH;   // 上次按键状态
int buttonState;              // 当前按键状态

unsigned long lastDebounceTime = 0;  // 上次抖动时间

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, ledState);
  
  Serial.begin(9600);
  Serial.println("按键切换 LED 实验（带消抖）");
}

void loop() {
  int reading = digitalRead(BUTTON_PIN);
  
  // 如果按键状态发生变化，重置消抖计时器
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  // 如果超过消抖时间，认为状态稳定
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != buttonState) {
      buttonState = reading;
      
      // 只在按键按下时（下降沿）触发
      if (buttonState == LOW) {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
        Serial.print("LED 状态切换为: ");
        Serial.println(ledState ? "亮" : "灭");
      }
    }
  }
  
  lastButtonState = reading;
}