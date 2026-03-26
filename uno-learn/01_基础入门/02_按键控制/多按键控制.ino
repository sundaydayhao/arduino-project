/*
 * 实验名称：多按键控制多 LED
 * 实验目的：使用多个按键分别控制多个 LED
 * 
 * 硬件连接：
 *   按键1 → D2，按键2 → D3，按键3 → D4
 *   LED1 → D8，LED2 → D9，LED3 → D10
 *   所有按键另一端 → GND
 *   所有 LED 通过 220Ω 电阻接 GND
 */

// 按键和 LED 数量
const int NUM_BUTTONS = 3;
const int NUM_LEDS = 3;

// 引脚定义
const int BUTTON_PINS[NUM_BUTTONS] = {2, 3, 4};
const int LED_PINS[NUM_LEDS] = {8, 9, 10};

// 消抖相关
const unsigned long DEBOUNCE_DELAY = 50;
int lastButtonStates[NUM_BUTTONS];
int buttonStates[NUM_BUTTONS];
unsigned long lastDebounceTimes[NUM_BUTTONS];
bool ledStates[NUM_LEDS];

void setup() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    lastButtonStates[i] = HIGH;
    buttonStates[i] = HIGH;
    lastDebounceTimes[i] = 0;
  }
  
  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    ledStates[i] = false;
    digitalWrite(LED_PINS[i], LOW);
  }
  
  Serial.begin(9600);
  Serial.println("多按键控制实验开始！");
  Serial.println("按键1控制LED1，按键2控制LED2，按键3控制LED3");
}

void loop() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    int reading = digitalRead(BUTTON_PINS[i]);
    
    if (reading != lastButtonStates[i]) {
      lastDebounceTimes[i] = millis();
    }
    
    if ((millis() - lastDebounceTimes[i]) > DEBOUNCE_DELAY) {
      if (reading != buttonStates[i]) {
        buttonStates[i] = reading;
        
        if (buttonStates[i] == LOW) {
          ledStates[i] = !ledStates[i];
          digitalWrite(LED_PINS[i], ledStates[i]);
          Serial.print("按键");
          Serial.print(i + 1);
          Serial.print(" → LED");
          Serial.print(i + 1);
          Serial.println(ledStates[i] ? " 亮" : " 灭");
        }
      }
    }
    
    lastButtonStates[i] = reading;
  }
}