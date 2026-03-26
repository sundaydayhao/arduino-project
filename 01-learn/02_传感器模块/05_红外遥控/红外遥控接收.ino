/*
 * 实验名称：红外遥控接收
 * 实验目的：接收红外遥控信号，解码并控制 LED
 * 
 * 需要安装库：IRremote（by shirriff）
 * 安装方法：Arduino IDE → 工具 → 管理库 → 搜索 "IRremote"
 * 
 * 硬件连接：
 *   红外接收模块：
 *     VCC → 5V
 *     GND → GND
 *     OUT → D11
 *   LED → D13
 * 
 * 常见遥控器按键编码（NEC 协议）：
 *   不同遥控器编码不同，先运行此程序查看你的遥控器编码
 */

#include <IRremote.h>

const int IR_PIN  = 11;
const int LED_PIN = 13;

IRrecv irrecv(IR_PIN);
decode_results results;

// 遥控器按键编码（根据实际遥控器修改）
// 先运行程序，按下按键查看串口输出的编码
const unsigned long BTN_POWER = 0xFFA25D;  // 电源键
const unsigned long BTN_UP    = 0xFF629D;  // 上键
const unsigned long BTN_DOWN  = 0xFFA857;  // 下键
const unsigned long BTN_LEFT  = 0xFF22DD;  // 左键
const unsigned long BTN_RIGHT = 0xFFC23D;  // 右键
const unsigned long BTN_OK    = 0xFF02FD;  // 确认键

bool ledState = false;
int brightness = 128;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  analogWrite(LED_PIN, brightness);
  
  irrecv.enableIRIn();  // 启动红外接收
  
  Serial.begin(9600);
  Serial.println("红外遥控接收实验");
  Serial.println("按下遥控器按键查看编码");
  Serial.println("电源键: 开关LED | 上/下键: 调节亮度");
}

void loop() {
  if (irrecv.decode(&results)) {
    unsigned long code = results.value;
    
    // 忽略重复码
    if (code != 0xFFFFFFFF) {
      Serial.print("收到编码: 0x");
      Serial.print(code, HEX);
      Serial.print(" | 协议: ");
      Serial.println(results.decode_type);
      
      // 处理按键
      if (code == BTN_POWER) {
        ledState = !ledState;
        if (!ledState) analogWrite(LED_PIN, 0);
        else analogWrite(LED_PIN, brightness);
        Serial.println(ledState ? "LED 开" : "LED 关");
      }
      else if (code == BTN_UP && ledState) {
        brightness = min(255, brightness + 25);
        analogWrite(LED_PIN, brightness);
        Serial.print("亮度: ");
        Serial.println(brightness);
      }
      else if (code == BTN_DOWN && ledState) {
        brightness = max(0, brightness - 25);
        analogWrite(LED_PIN, brightness);
        Serial.print("亮度: ");
        Serial.println(brightness);
      }
    }
    
    irrecv.resume();  // 准备接收下一个信号
  }
}