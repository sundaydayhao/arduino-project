# DS3231 RTC 实时时钟实验

## 简介
DS3231 是一款高精度 I2C 实时时钟芯片，内置温度补偿晶振，精度 ±2ppm（每天误差 < 0.2秒）。

## 硬件连接
```
DS3231    Arduino UNO
VCC  ──── 3.3V 或 5V
GND  ──── GND
SDA  ──── A4
SCL  ──── A5
```

## 需要安装的库
- **RTClib**（by Adafruit）

## 实验文件
1. `RTC时钟.ino` - 读取时间、设置时间、读取温度

## 常用函数
```cpp
rtc.begin();                    // 初始化
rtc.adjust(DateTime(...));      // 设置时间
rtc.now();                      // 获取当前时间
now.year() / month() / day()    // 年月日
now.hour() / minute() / second()// 时分秒
now.dayOfTheWeek()              // 星期（0=周日）
rtc.getTemperature()            // 读取温度
rtc.lostPower()                 // 是否断电过
```

## 思考题
1. 如何结合 RTC 和 SD 卡实现带时间戳的数据记录？
2. 如何实现定时闹钟功能？
3. DS1307 和 DS3231 有什么区别？