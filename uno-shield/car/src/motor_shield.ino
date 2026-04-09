/**
 * ============================================================
 * Arduino 电机驱动 Shield（TB6612FNG）
 * Motor Driver Shield - TB6612FNG Dual DC Motor + Servo
 * ============================================================
 *
 * 功能 / Features:
 *   - TB6612FNG 双路直流电机驱动（1.2A/通道，峰值 3.2A）
 *   - PWM 调速（0–255）
 *   - 4 路舵机输出（Servo 库）
 *   - 编码器读取（2路，中断驱动）
 *   - 串口命令控制（前进/后退/左转/右转/停止/速度）
 *   - 小车 Demo 模式（自动演示动作序列）
 *
 * 硬件连接 / Wiring:
 *   TB6612FNG 模块:
 *     PWMA -> Pin 3  (电机A速度，PWM)
 *     AIN1 -> Pin 4  (电机A方向1)
 *     AIN2 -> Pin 5  (电机A方向2)
 *     PWMB -> Pin 9  (电机B速度，PWM)
 *     BIN1 -> Pin 7  (电机B方向1)
 *     BIN2 -> Pin 8  (电机B方向2)
 *     STBY -> Pin 6  (待机控制，HIGH=使能)
 *     VCC  -> 5V
 *     GND  -> GND
 *     VM   -> 电机电源（6–15V，独立供电）
 *
 *   舵机:
 *     舵机1 -> Pin 10 (PWM)
 *     舵机2 -> Pin 11 (PWM)
 *     舵机3 -> Pin 12 (PWM)
 *     舵机4 -> Pin 13 (PWM)
 *
 *   编码器A（电机A）:
 *     CLK  -> Pin 2  (INT0，计数脉冲)
 *     DIR  -> A0     (方向信号)
 *
 *   编码器B（电机B）:
 *     CLK  -> Pin A1 (轮询读取)
 *     DIR  -> A2
 *
 * 串口命令 / Serial Commands (9600 baud):
 *   F        前进 Forward
 *   B        后退 Backward
 *   L        左转 Turn Left
 *   R        右转 Turn Right
 *   S        停止 Stop
 *   V<0-255> 设置速度 (如 V180)
 *   S1<0-180> 舵机1角度 (如 S190)
 *   S2<0-180> 舵机2角度
 *   S3<0-180> 舵机3角度
 *   S4<0-180> 舵机4角度
 *   D        Demo 模式（自动演示）
 *   E        打印编码器计数
 *   ?        帮助
 * ============================================================
 */

#include <Servo.h>

// ─── 引脚定义 / Pin Definitions ──────────────────────────────
// 电机 A
#define PIN_PWMA   3
#define PIN_AIN1   4
#define PIN_AIN2   5
// 电机 B
#define PIN_PWMB   9
#define PIN_BIN1   7
#define PIN_BIN2   8
// 待机
#define PIN_STBY   6

// 舵机
#define PIN_SERVO1  10
#define PIN_SERVO2  11
#define PIN_SERVO3  12
#define PIN_SERVO4  13

// 编码器 A（中断）
#define PIN_ENC_A_CLK  2   // INT0
#define PIN_ENC_A_DIR  A0

// 编码器 B（轮询）
#define PIN_ENC_B_CLK  A1
#define PIN_ENC_B_DIR  A2

// ─── 配置参数 / Configuration ─────────────────────────────────
#define DEFAULT_SPEED   180    // 默认速度 (0–255)
#define TURN_SPEED      150    // 转弯速度
#define DEBOUNCE_MS     200

// ─── 对象实例化 / Object Instantiation ───────────────────────
Servo servo1, servo2, servo3, servo4;

// ─── 全局状态 / Global State ──────────────────────────────────
int  motorSpeed    = DEFAULT_SPEED;
bool motorsEnabled = false;

// 编码器计数（volatile，中断修改）
volatile long encA_count = 0;
long          encB_count = 0;
int           encB_lastClk = HIGH;

// ─── 编码器A中断 / Encoder A ISR ─────────────────────────────
void ISR_encA() {
  if (digitalRead(PIN_ENC_A_DIR) == HIGH) {
    encA_count++;
  } else {
    encA_count--;
  }
}

// ─── 电机控制函数 / Motor Control ────────────────────────────

// 使能/禁用 TB6612FNG
void setStandby(bool standby) {
  digitalWrite(PIN_STBY, standby ? LOW : HIGH);
  motorsEnabled = !standby;
}

// 电机A控制：speed -255~255（负数=反转）
void motorA(int speed) {
  speed = constrain(speed, -255, 255);
  if (speed > 0) {
    digitalWrite(PIN_AIN1, HIGH);
    digitalWrite(PIN_AIN2, LOW);
    analogWrite(PIN_PWMA, speed);
  } else if (speed < 0) {
    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, HIGH);
    analogWrite(PIN_PWMA, -speed);
  } else {
    // 刹车
    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, LOW);
    analogWrite(PIN_PWMA, 0);
  }
}

// 电机B控制：speed -255~255（负数=反转）
void motorB(int speed) {
  speed = constrain(speed, -255, 255);
  if (speed > 0) {
    digitalWrite(PIN_BIN1, HIGH);
    digitalWrite(PIN_BIN2, LOW);
    analogWrite(PIN_PWMB, speed);
  } else if (speed < 0) {
    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, HIGH);
    analogWrite(PIN_PWMB, -speed);
  } else {
    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, LOW);
    analogWrite(PIN_PWMB, 0);
  }
}

// 小车运动控制
void carForward(int speed) {
  setStandby(false);
  motorA(speed);
  motorB(speed);
  Serial.print(F("[FWD] speed=")); Serial.println(speed);
}

void carBackward(int speed) {
  setStandby(false);
  motorA(-speed);
  motorB(-speed);
  Serial.print(F("[BWD] speed=")); Serial.println(speed);
}

void carLeft(int speed) {
  setStandby(false);
  motorA(-speed);   // 左轮反转
  motorB(speed);    // 右轮正转
  Serial.print(F("[LEFT] speed=")); Serial.println(speed);
}

void carRight(int speed) {
  setStandby(false);
  motorA(speed);    // 左轮正转
  motorB(-speed);   // 右轮反转
  Serial.print(F("[RIGHT] speed=")); Serial.println(speed);
}

void carStop() {
  motorA(0);
  motorB(0);
  setStandby(true);
  Serial.println(F("[STOP]"));
}

// ─── 编码器B轮询 / Encoder B Polling ─────────────────────────
void pollEncoderB() {
  int clk = digitalRead(PIN_ENC_B_CLK);
  if (clk != encB_lastClk && clk == LOW) {
    if (digitalRead(PIN_ENC_B_DIR) == HIGH) {
      encB_count++;
    } else {
      encB_count--;
    }
  }
  encB_lastClk = clk;
}

// ─── Demo 模式 / Demo Mode ────────────────────────────────────
void runDemo() {
  Serial.println(F("[DEMO] Starting demo sequence..."));

  // 前进 1 秒
  carForward(motorSpeed);
  delay(1000);

  // 左转 0.5 秒
  carLeft(TURN_SPEED);
  delay(500);

  // 前进 1 秒
  carForward(motorSpeed);
  delay(1000);

  // 右转 0.5 秒
  carRight(TURN_SPEED);
  delay(500);

  // 后退 1 秒
  carBackward(motorSpeed);
  delay(1000);

  // 原地旋转 1 秒
  carLeft(motorSpeed);
  delay(1000);

  // 停止
  carStop();

  // 舵机扫描演示
  Serial.println(F("[DEMO] Servo sweep..."));
  for (int angle = 0; angle <= 180; angle += 10) {
    servo1.write(angle);
    servo2.write(180 - angle);
    delay(50);
  }
  for (int angle = 180; angle >= 0; angle -= 10) {
    servo1.write(angle);
    servo2.write(180 - angle);
    delay(50);
  }
  servo1.write(90);
  servo2.write(90);

  Serial.println(F("[DEMO] Done."));
}

// ─── 串口命令处理 / Serial Command Handler ────────────────────
void handleSerial() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd.length() == 0) return;

  char c = cmd.charAt(0);

  switch (c) {
    case 'F': case 'f':
      carForward(motorSpeed);
      break;

    case 'B': case 'b':
      carBackward(motorSpeed);
      break;

    case 'L': case 'l':
      carLeft(TURN_SPEED);
      break;

    case 'R': case 'r':
      carRight(TURN_SPEED);
      break;

    case 'S': case 's':
      // S 单独 = 停止；S1/S2/S3/S4 = 舵机
      if (cmd.length() == 1) {
        carStop();
      } else {
        int servoNum = cmd.charAt(1) - '0';
        int angle    = cmd.substring(2).toInt();
        angle = constrain(angle, 0, 180);
        switch (servoNum) {
          case 1: servo1.write(angle); Serial.print(F("[S1] ")); break;
          case 2: servo2.write(angle); Serial.print(F("[S2] ")); break;
          case 3: servo3.write(angle); Serial.print(F("[S3] ")); break;
          case 4: servo4.write(angle); Serial.print(F("[S4] ")); break;
          default: Serial.println(F("[ERR] Servo 1-4 only")); return;
        }
        Serial.println(angle);
      }
      break;

    case 'V': case 'v': {
      int spd = cmd.substring(1).toInt();
      motorSpeed = constrain(spd, 0, 255);
      Serial.print(F("[SPEED] ")); Serial.println(motorSpeed);
      break;
    }

    case 'D': case 'd':
      runDemo();
      break;

    case 'E': case 'e':
      Serial.print(F("[ENC] A="));
      Serial.print(encA_count);
      Serial.print(F("  B="));
      Serial.println(encB_count);
      break;

    case '0':
      // 重置编码器
      encA_count = 0;
      encB_count = 0;
      Serial.println(F("[ENC] Reset to 0"));
      break;

    default:
      Serial.println(F("=== Commands ==="));
      Serial.println(F("F        Forward"));
      Serial.println(F("B        Backward"));
      Serial.println(F("L        Turn Left"));
      Serial.println(F("R        Turn Right"));
      Serial.println(F("S        Stop"));
      Serial.println(F("V<0-255> Set speed (e.g. V180)"));
      Serial.println(F("S1<0-180> Servo1 angle (e.g. S190)"));
      Serial.println(F("S2/S3/S4  Same for servo 2/3/4"));
      Serial.println(F("D        Demo mode"));
      Serial.println(F("E        Print encoder counts"));
      Serial.println(F("0        Reset encoder counts"));
      break;
  }
}

// ─── Setup ────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  Serial.println(F("=== Motor Driver Shield (TB6612FNG) ==="));

  // 电机引脚
  pinMode(PIN_PWMA, OUTPUT);
  pinMode(PIN_AIN1, OUTPUT);
  pinMode(PIN_AIN2, OUTPUT);
  pinMode(PIN_PWMB, OUTPUT);
  pinMode(PIN_BIN1, OUTPUT);
  pinMode(PIN_BIN2, OUTPUT);
  pinMode(PIN_STBY, OUTPUT);

  // 编码器引脚
  pinMode(PIN_ENC_A_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC_A_DIR, INPUT_PULLUP);
  pinMode(PIN_ENC_B_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC_B_DIR, INPUT_PULLUP);

  // 初始状态：待机（电机关闭）
  carStop();

  // 编码器A中断（下降沿）
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A_CLK), ISR_encA, FALLING);

  // 舵机初始化（90° 中位）
  servo1.attach(PIN_SERVO1);
  servo2.attach(PIN_SERVO2);
  servo3.attach(PIN_SERVO3);
  servo4.attach(PIN_SERVO4);
  servo1.write(90);
  servo2.write(90);
  servo3.write(90);
  servo4.write(90);

  Serial.println(F("Ready. Send '?' for help."));
  Serial.print(F("Default speed: ")); Serial.println(motorSpeed);
  Serial.println(F("Servos 1-4 initialized to 90 degrees."));
}

// ─── Loop ─────────────────────────────────────────────────────
void loop() {
  pollEncoderB();
  handleSerial();
}