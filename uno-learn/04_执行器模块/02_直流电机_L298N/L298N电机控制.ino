/*
 * 实验名称：L298N 直流电机控制
 * 实验目的：学习使用 L298N 驱动模块控制直流电机
 * 
 * 硬件连接：
 *   L298N 模块：
 *     IN1 → D8
 *     IN2 → D9（PWM）
 *     IN3 → D10（PWM）
 *     IN4 → D11
 *     ENA → D9（PWM，控制电机A速度）
 *     ENB → D10（PWM，控制电机B速度）
 *     VCC → 外部电源（6~12V）
 *     GND → 外部电源GND + Arduino GND
 *     5V  → Arduino 5V（可选，为Arduino供电）
 * 
 * ⚠️ 必须使用外部电源驱动电机！
 * 
 * 知识点：
 *   - H桥电路原理
 *   - PWM 调速
 *   - 电机正反转控制
 */

// 电机A引脚
const int IN1 = 8;
const int IN2 = 9;   // PWM
const int ENA = 9;   // 与IN2共用（简化接线）

// 电机B引脚
const int IN3 = 10;  // PWM
const int IN4 = 11;
const int ENB = 10;  // 与IN3共用

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  
  Serial.begin(9600);
  Serial.println("L298N 电机控制实验");
}

void loop() {
  // 电机A正转，速度 150
  Serial.println("电机A 正转 速度150");
  motorA(150, true);
  delay(2000);
  
  // 电机A反转，速度 200
  Serial.println("电机A 反转 速度200");
  motorA(200, false);
  delay(2000);
  
  // 停止
  Serial.println("停止");
  motorA(0, true);
  delay(1000);
  
  // 加速
  Serial.println("加速...");
  for (int speed = 0; speed <= 255; speed += 5) {
    motorA(speed, true);
    Serial.print("速度: ");
    Serial.println(speed);
    delay(50);
  }
  
  // 减速
  Serial.println("减速...");
  for (int speed = 255; speed >= 0; speed -= 5) {
    motorA(speed, true);
    delay(50);
  }
  
  delay(1000);
}

// 控制电机A
// speed: 0~255
// forward: true=正转, false=反转
void motorA(int speed, bool forward) {
  speed = constrain(speed, 0, 255);
  if (speed == 0) {
    // 刹车
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  } else if (forward) {
    digitalWrite(IN1, HIGH);
    analogWrite(IN2, 255 - speed);  // IN2 为 PWM 引脚
  } else {
    analogWrite(IN1, 255 - speed);  // 注意：这里需要调整
    digitalWrite(IN2, HIGH);
  }
}

// 控制电机B
void motorB(int speed, bool forward) {
  speed = constrain(speed, 0, 255);
  if (speed == 0) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  } else if (forward) {
    analogWrite(IN3, speed);
    digitalWrite(IN4, LOW);
  } else {
    digitalWrite(IN3, LOW);
    analogWrite(IN4, speed);
  }
}