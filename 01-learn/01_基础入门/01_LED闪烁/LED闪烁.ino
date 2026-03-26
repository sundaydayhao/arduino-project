/*
 * 实验名称：LED 闪烁
 * 实验目的：学习 Arduino 数字输出，控制 LED 闪烁
 * 
 * 硬件连接：
 *   LED 正极 → 220Ω 电阻 → Arduino D13
 *   LED 负极 → Arduino GND
 * 
 * 知识点：
 *   - pinMode()  设置引脚模式
 *   - digitalWrite() 数字输出
 *   - delay() 延时函数
 */

// 定义 LED 连接的引脚
const int LED_PIN = 13;

void setup() {
  // 将 LED 引脚设置为输出模式
  pinMode(LED_PIN, OUTPUT);
  
  // 初始化串口，波特率 9600
  Serial.begin(9600);
  Serial.println("LED 闪烁实验开始！");
}

void loop() {
  // 点亮 LED
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED 亮");
  delay(1000);  // 等待 1 秒
  
  // 熄灭 LED
  digitalWrite(LED_PIN, LOW);
  Serial.println("LED 灭");
  delay(1000);  // 等待 1 秒
}