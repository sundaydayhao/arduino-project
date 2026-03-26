/*
 * 实验名称：HC-05 蓝牙通信
 * 实验目的：通过蓝牙与手机通信，实现无线控制
 * 
 * 硬件连接：
 *   HC-05 VCC → 5V
 *   HC-05 GND → GND
 *   HC-05 TXD → Arduino D10（软串口RX）
 *   HC-05 RXD → Arduino D11（软串口TX，需要分压！）
 * 
 * ⚠️ 注意：HC-05 RXD 引脚为 3.3V 电平！
 *    Arduino TX(5V) → 1kΩ → HC-05 RXD → 2kΩ → GND
 *    （分压到约 3.3V）
 * 
 * 手机端：下载 "Serial Bluetooth Terminal" 或 "Arduino Bluetooth Controller"
 * 默认配对密码：1234 或 0000
 * 
 * 知识点：
 *   - 软件串口（SoftwareSerial）
 *   - 蓝牙 SPP 协议
 *   - AT 命令配置 HC-05
 */

#include <SoftwareSerial.h>

// 软件串口：RX=D10, TX=D11
SoftwareSerial bluetooth(10, 11);

const int LED_PIN = 13;
bool ledState = false;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  
  Serial.begin(9600);      // 硬件串口（调试用）
  bluetooth.begin(9600);   // 蓝牙串口（默认波特率9600）
  
  Serial.println("HC-05 蓝牙实验");
  Serial.println("等待蓝牙连接...");
  bluetooth.println("Arduino 蓝牙已就绪！");
  bluetooth.println("命令: LED_ON / LED_OFF / STATUS");
}

void loop() {
  // 接收蓝牙数据
  if (bluetooth.available()) {
    String cmd = bluetooth.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();
    
    Serial.print("蓝牙收到: ");
    Serial.println(cmd);
    
    if (cmd == "LED_ON" || cmd == "1") {
      ledState = true;
      digitalWrite(LED_PIN, HIGH);
      bluetooth.println("LED 已开启");
    }
    else if (cmd == "LED_OFF" || cmd == "0") {
      ledState = false;
      digitalWrite(LED_PIN, LOW);
      bluetooth.println("LED 已关闭");
    }
    else if (cmd == "STATUS") {
      bluetooth.print("LED: ");
      bluetooth.println(ledState ? "ON" : "OFF");
      bluetooth.print("运行时间: ");
      bluetooth.print(millis() / 1000);
      bluetooth.println("s");
    }
    else {
      bluetooth.print("未知命令: ");
      bluetooth.println(cmd);
    }
  }
  
  // 将硬件串口数据发送到蓝牙（调试用）
  if (Serial.available()) {
    bluetooth.write(Serial.read());
  }
}