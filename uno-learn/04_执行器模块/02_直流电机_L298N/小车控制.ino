/*
 * 实验名称：双轮小车控制
 * 实验目的：使用 L298N 控制双轮小车前进、后退、转向
 * 
 * 硬件连接：
 *   左电机：IN1→D8, IN2→D9(PWM)
 *   右电机：IN3→D10(PWM), IN4→D11
 *   L298N VCC → 外部电源（7.4V 锂电池推荐）
 *   L298N GND → 外部GND + Arduino GND
 * 
 * 串口命令：
 *   'w' - 前进
 *   's' - 后退
 *   'a' - 左转
 *   'd' - 右转
 *   ' ' - 停止
 *   '+' - 加速
 *   '-' - 减速
 */

// 左电机
const int LEFT_IN1 = 8;
const int LEFT_IN2 = 9;

// 右电机
const int RIGHT_IN3 = 10;
const int RIGHT_IN4 = 11;

int carSpeed = 150;  // 默认速度

void setup() {
  pinMode(LEFT_IN1, OUTPUT);
  pinMode(LEFT_IN2, OUTPUT);
  pinMode(RIGHT_IN3, OUTPUT);
  pinMode(RIGHT_IN4, OUTPUT);
  
  stopCar();
  
  Serial.begin(9600);
  Serial.println("=== 小车控制 ===");
  Serial.println("w=前进 s=后退 a=左转 d=右转 空格=停止");
  Serial.println("+/-=调速");
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    
    switch (cmd) {
      case 'w': case 'W':
        forward();
        Serial.print("前进 速度:");
        Serial.println(carSpeed);
        break;
      case 's': case 'S':
        backward();
        Serial.print("后退 速度:");
        Serial.println(carSpeed);
        break;
      case 'a': case 'A':
        turnLeft();
        Serial.println("左转");
        break;
      case 'd': case 'D':
        turnRight();
        Serial.println("右转");
        break;
      case ' ':
        stopCar();
        Serial.println("停止");
        break;
      case '+':
        carSpeed = min(255, carSpeed + 20);
        Serial.print("速度: ");
        Serial.println(carSpeed);
        break;
      case '-':
        carSpeed = max(50, carSpeed - 20);
        Serial.print("速度: ");
        Serial.println(carSpeed);
        break;
    }
  }
}

void forward() {
  analogWrite(LEFT_IN1, carSpeed);
  digitalWrite(LEFT_IN2, LOW);
  analogWrite(RIGHT_IN3, carSpeed);
  digitalWrite(RIGHT_IN4, LOW);
}

void backward() {
  digitalWrite(LEFT_IN1, LOW);
  analogWrite(LEFT_IN2, carSpeed);
  digitalWrite(RIGHT_IN3, LOW);
  analogWrite(RIGHT_IN4, carSpeed);
}

void turnLeft() {
  digitalWrite(LEFT_IN1, LOW);
  analogWrite(LEFT_IN2, carSpeed);
  analogWrite(RIGHT_IN3, carSpeed);
  digitalWrite(RIGHT_IN4, LOW);
}

void turnRight() {
  analogWrite(LEFT_IN1, carSpeed);
  digitalWrite(LEFT_IN2, LOW);
  digitalWrite(RIGHT_IN3, LOW);
  analogWrite(RIGHT_IN4, carSpeed);
}

void stopCar() {
  digitalWrite(LEFT_IN1, LOW);
  digitalWrite(LEFT_IN2, LOW);
  digitalWrite(RIGHT_IN3, LOW);
  digitalWrite(RIGHT_IN4, LOW);
}