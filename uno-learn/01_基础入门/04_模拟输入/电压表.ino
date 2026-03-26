/*
 * 实验名称：简易电压表
 * 实验目的：用 Arduino 测量 0~5V 电压
 * 
 * 硬件连接：
 *   待测电压正极 → A0（注意：不能超过 5V！）
 *   待测电压负极 → GND
 * 
 * ⚠️ 警告：A0 引脚最大承受 5V，超过会损坏 Arduino！
 *    测量更高电压需要使用分压电路。
 * 
 * 知识点：
 *   - Arduino ADC 精度：10位 = 1024个等级
 *   - 分辨率：5V / 1024 ≈ 4.88mV
 *   - 使用分压电路可测量更高电压
 */

const int VOLT_PIN = A0;
const float VREF = 5.0;      // 参考电压
const int ADC_MAX = 1023;    // ADC 最大值

// 如果使用分压电路（R1=30kΩ, R2=10kΩ），最大可测 20V
// const float DIVIDER_RATIO = 4.0;
const float DIVIDER_RATIO = 1.0;  // 直接测量，最大 5V

// 滑动平均滤波
const int SAMPLES = 10;
int readings[SAMPLES];
int readIndex = 0;
long total = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("简易电压表");
  Serial.println("测量范围: 0 ~ 5V");
  Serial.println("分辨率: ~4.88mV");
  
  // 初始化滤波数组
  for (int i = 0; i < SAMPLES; i++) {
    readings[i] = 0;
  }
}

void loop() {
  // 滑动平均滤波
  total -= readings[readIndex];
  readings[readIndex] = analogRead(VOLT_PIN);
  total += readings[readIndex];
  readIndex = (readIndex + 1) % SAMPLES;
  
  int avgReading = total / SAMPLES;
  
  // 计算电压
  float voltage = (avgReading / (float)ADC_MAX) * VREF * DIVIDER_RATIO;
  
  Serial.print("ADC: ");
  Serial.print(avgReading);
  Serial.print(" | 电压: ");
  Serial.print(voltage, 3);
  Serial.println(" V");
  
  delay(200);
}