/*
 * 实验名称：HC-SR04 超声波测距
 * 实验目的：使用超声波传感器测量距离
 * 
 * 硬件连接：
 *   VCC → 5V
 *   GND → GND
 *   Trig → D9
 *   Echo → D10
 * 
 * 工作原理：
 *   1. Trig 发送 10μs 高电平脉冲
 *   2. 传感器发出 8 个 40kHz 超声波
 *   3. Echo 引脚高电平持续时间 = 声波往返时间
 *   4. 距离 = 时间 × 声速(340m/s) / 2
 * 
 * 测量范围：2cm ~ 400cm
 * 精度：±3mm
 */

const int TRIG_PIN = 9;
const int ECHO_PIN = 10;

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Serial.begin(9600);
  Serial.println("HC-SR04 超声波测距实验");
  Serial.println("测量范围: 2cm ~ 400cm");
}

void loop() {
  float distance = measureDistance();
  
  if (distance < 0) {
    Serial.println("超出测量范围！");
  } else {
    Serial.print("距离: ");
    Serial.print(distance, 1);
    Serial.println(" cm");
  }
  
  delay(200);
}

// 测量距离（返回厘米，-1 表示超出范围）
float measureDistance() {
  // 发送触发脉冲
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // 读取回波时间（超时 30ms）
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  
  if (duration == 0) {
    return -1;  // 超时，超出范围
  }
  
  // 计算距离：声速 340m/s = 0.034cm/μs，除以2（往返）
  float distance = duration * 0.034 / 2.0;
  
  // 检查有效范围
  if (distance < 2 || distance > 400) {
    return -1;
  }
  
  return distance;
}