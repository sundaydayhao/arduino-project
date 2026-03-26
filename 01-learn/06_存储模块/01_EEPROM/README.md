# EEPROM 存储实验

## 简介
EEPROM（电可擦可编程只读存储器）是 Arduino 内置的非易失性存储器，掉电后数据不丢失。

## Arduino EEPROM 容量
| 型号 | EEPROM 大小 |
|------|------------|
| UNO / Nano | 1 KB |
| Mega 2560 | 4 KB |
| Leonardo | 1 KB |

## 注意事项
- ⚠️ EEPROM 写入寿命约 **10 万次**，避免在 loop() 中频繁写入
- 读取次数无限制
- 使用 `EEPROM.update()` 代替 `write()` 可减少写入次数（只在值变化时写入）

## 实验文件
1. `EEPROM读写.ino` - 串口命令读写EEPROM

## 常用函数
```cpp
EEPROM.read(addr);          // 读取一个字节
EEPROM.write(addr, val);    // 写入一个字节
EEPROM.update(addr, val);   // 仅在值变化时写入
EEPROM.get(addr, data);     // 读取任意类型数据
EEPROM.put(addr, data);     // 写入任意类型数据
EEPROM.length();            // 返回 EEPROM 大小