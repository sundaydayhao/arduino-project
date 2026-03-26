# 串口通信实验

## 实验简介
学习 Arduino 与电脑之间的 UART 串口通信，实现双向数据传输。

## 硬件清单
| 元件 | 数量 |
|------|------|
| Arduino UNO | 1 |
| USB 数据线 | 1 |
| LED（可选） | 3 |
| 220Ω 电阻（可选） | 3 |

## 如何使用串口监视器

1. 上传代码后，点击 Arduino IDE 右上角的串口监视器图标
2. 确保波特率与代码中 `Serial.begin()` 的参数一致
3. 在输入框输入命令，按回车发送

## 实验文件
1. `串口基础.ino` - 串口收发基础操作
2. `串口控制LED.ino` - 通过串口命令控制 LED

## 重要函数说明

```cpp
Serial.begin(9600);        // 初始化串口，设置波特率
Serial.print("text");      // 发送文本（不换行）
Serial.println("text");    // 发送文本（换行）
Serial.print(value, DEC);  // 发送数字（十进制）
Serial.print(value, HEX);  // 发送数字（十六进制）
Serial.available();        // 返回可读字节数
Serial.read();             // 读取一个字节
Serial.readString();       // 读取字符串（到超时）
Serial.parseInt();         // 读取整数
```

## 常用波特率
`300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200`

> 推荐使用 9600 或 115200

## 思考题
1. 波特率不匹配会出现什么现象？
2. 如何实现 Arduino 与 Arduino 之间的串口通信？
3. `Serial.print()` 和 `Serial.write()` 有什么区别？