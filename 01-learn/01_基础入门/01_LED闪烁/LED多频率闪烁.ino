/*
 * 实验名称：LED 多频率闪烁（进阶）
 * 实验目的：使用 millis() 实现非阻塞延时，控制多个 LED 以不同频率闪烁
 * 
 * 硬件连接：
 *   LED1 正极 → 220Ω 电阻 → Arduino D8
 *   LED2 正极 → 220Ω 电阻 → Arduino D9
 *   LED3 正极 → 220Ω 电阻 → Arduino D10
 *   所有 LED 负极 → Arduino GND
 * 
 * 知识点：
 *   - millis() 获取运行时间（非阻塞延时）
 *   - 多任务并行处理思想
 */

// LED 引脚定义
const int LED1 = 8;
const int LED2 = 9;
const int LED3 = 10;

// 各 LED 闪烁间隔（毫秒）
const long INTERVAL1 = 500;   // LED1: 0.5秒
const long INTERVAL2 = 1000;  // LED2: 1秒
const long INTERVAL3 = 2000;  // LED3: 2秒

// 记录上次切换时间
unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;
unsigned long previousMillis3 = 0;

// LED 状态
bool ledState1 = LOW;
bool ledState2 = LOW;
bool ledState3 = LOW;

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  Serial.begin(9600);
  Serial.println("多频率 LED 闪烁实验开始！");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // 控制 LED1
  if (currentMillis - previousMillis1 >= INTERVAL1) {
    previousMillis1 = currentMillis;
    ledState1 = !ledState1;
    digitalWrite(LED1, ledState1);
  }
  
  // 控制 LED2
  if (currentMillis - previousMillis2 >= INTERVAL2) {
    previousMillis2 = currentMillis;
    ledState2 = !ledState2;
    digitalWrite(LED2, ledState2);
  }
  
  // 控制 LED3
  if (currentMillis - previousMillis3 >= INTERVAL3) {
    previousMillis3 = currentMillis;
    ledState3 = !ledState3;
    digitalWrite(LED3, ledState3);
  }
}