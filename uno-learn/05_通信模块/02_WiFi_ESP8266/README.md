# ESP8266 WiFi 通信实验

## 模块简介
ESP8266 是一款低成本 WiFi 模块，可让 Arduino 接入互联网。

## 硬件连接（ESP-01）
```
ESP-01    Arduino/电源
VCC  ──── 3.3V稳压（不能用Arduino 3.3V引脚！）
GND  ──── GND
TX   ──── D10（SoftwareSerial RX）
RX   ──── D11（需分压到3.3V）
CH_PD ─── 3.3V
```

## 注意事项
- ⚠️ ESP8266 工作电压 **3.3V**，需要独立稳压电源（AMS1117-3.3）
- ⚠️ 工作电流约 **300mA**，Arduino 3.3V 引脚无法提供

## 实验文件
1. `ESP8266_WiFi.ino` - 连接WiFi并发送数据到ThingSpeak

## 思考题
1. 如何用 ESP8266 搭建一个简单的 Web 服务器？
2. ESP8266 和 ESP32 有什么区别？
3. 如何实现 MQTT 协议通信？