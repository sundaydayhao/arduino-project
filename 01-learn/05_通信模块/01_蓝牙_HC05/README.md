# HC-05 蓝牙通信实验

## 模块简介
HC-05 是一款常用的蓝牙 2.0 串口模块，支持主从模式，可与手机、电脑等设备通信。

## 规格参数
| 参数 | 值 |
|------|-----|
| 蓝牙版本 | 2.0 + EDR |
| 工作电压 | 3.3V（逻辑电平） |
| 供电电压 | 3.6~6V |
| 默认波特率 | 9600 |
| AT模式波特率 | 38400 |
| 默认名称 | HC-05 |
| 默认密码 | 1234 |
| 通信距离 | ~10m |

## 硬件连接

```
HC-05          Arduino UNO
VCC ─────────── 5V
GND ─────────── GND
TXD ─────────── D10（SoftwareSerial RX）
RXD ─┬──────── D11（SoftwareSerial TX）
     │
    1kΩ（分压，5V→3.3V）
     │
    2kΩ
     │
    GND
```

> ⚠️ HC-05 的 RXD 是 3.3V 电平，Arduino TX 是 5V，必须分压！

## 手机 APP 推荐
- **Android**: Serial Bluetooth Terminal、Arduino Bluetooth Controller
- **iOS**: LightBlue（仅支持 BLE，HC-05 不支持）

## 需要安装的库
- **SoftwareSerial**（Arduino 内置）

## 实验文件
1. `HC05蓝牙通信.ino` - 蓝牙控制 LED
2. `HC05_AT配置.ino` - AT 命令配置工具

## 思考题
1. HC-05 和 HC-06 有什么区别？
2. 蓝牙 2.0 和蓝牙 4.0（BLE）有什么区别？
3. 如何实现两个 Arduino 之间的蓝牙通信？