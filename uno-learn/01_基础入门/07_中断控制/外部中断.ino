/*
 * 实验名称：外部中断
 * 实验目的：学习使用外部中断响应按键事件，理解中断机制
 * 
 * 硬件连接：
 *   按键 → Arduino D2（INT0）或 D3（INT1）
 *   按键另一端 → GND
 *   LED → D13
 * 
 * 知识点：
 *   - 中断：CPU 暂停当前任务，处理紧急事件
 *   - Arduino UNO 外部中断引脚：D2(INT0), D3(INT1)
 *   - attachInterrupt() 注册中断
 *   - volatile 关键字：告诉编译器变量可能被中断修改
 *   - 中断服务函数（ISR）要尽量短小
 */

const int BUTTON_PIN = 2;   // INT0
const int LED_PIN = 13;

volatile bool ledState = false;    // volatile：中断中修改的变量
volatile unsigned long lastInterruptTime = 0;

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  
  // 注册中断：引脚2，下降沿触发，调用 buttonISR 函数
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
  
  Serial.begin(9600);
  Serial.println("外部中断实验");
  Serial.println("按下按键触发中断，LED 状态切换");
}

void loop() {
  // 主循环可以做其他事情
  Serial.println("主循环运行中...");
  delay(2000);
  
  // 更新 LED 状态
  digitalWrite(LED_PIN, ledState);
}

// 中断服务函数（ISR）- 必须简短！
void buttonISR() {
  // 软件消抖：忽略 200ms 内的重复中断
  unsigned long currentTime = millis();
  if (currentTime - lastInterruptTime > 200) {
    ledState = !ledState;
    lastInterruptTime = currentTime;
  }
}