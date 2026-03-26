/*
 * 工具程序：HC-05 AT 命令配置
 * 用途：通过 AT 命令配置 HC-05 蓝牙模块
 * 
 * 进入 AT 模式：
 *   方法1：上电时按住 HC-05 上的按键
 *   方法2：将 KEY/EN 引脚接 HIGH 后上电
 * 
 * AT 模式下波特率：38400
 * 
 * 硬件连接：
 *   HC-05 TXD → D10
 *   HC-05 RXD → D11（需要分压）
 *   HC-05 KEY → 5V（进入AT模式）
 * 
 * 常用 AT 命令：
 *   AT              → 测试（返回 OK）
 *   AT+NAME?        → 查询名称
 *   AT+NAME=MyBT    → 设置名称
 *   AT+PSWD?        → 查询密码
 *   AT+PSWD=1234    → 设置密码
 *   AT+UART?        → 查询波特率
 *   AT+UART=9600,0,0 → 设置波特率
 *   AT+ROLE?        → 查询角色（0=从机，1=主机）
 *   AT+RESET        → 重启
 */

#include <SoftwareSerial.h>

SoftwareSerial bluetooth(10, 11);

void setup() {
  Serial.begin(9600);
  bluetooth.begin(38400);  // AT 模式波特率
  
  Serial.println("HC-05 AT 命令配置工具");
  Serial.println("在串口监视器输入 AT 命令，发送到 HC-05");
  Serial.println("注意：串口监视器选择 'Both NL & CR'");
  Serial.println("-----------------------------------");
  Serial.println("常用命令:");
  Serial.println("AT          - 测试连接");
  Serial.println("AT+NAME?    - 查询名称");
  Serial.println("AT+PSWD?    - 查询密码");
  Serial.println("AT+UART?    - 查询波特率");
  Serial.println("AT+VERSION? - 查询版本");
}

void loop() {
  // 将串口数据转发到蓝牙
  if (Serial.available()) {
    char c = Serial.read();
    bluetooth.write(c);
    Serial.write(c);  // 回显
  }
  
  // 将蓝牙数据转发到串口
  if (bluetooth.available()) {
    char c = bluetooth.read();
    Serial.write(c);
  }
}