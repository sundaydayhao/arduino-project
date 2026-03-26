# LED 闪烁实验

## 实验简介
这是 Arduino 最基础的实验，通过控制 LED 的亮灭来学习数字输出。

## 硬件清单
| 元件 | 数量 |
|------|------|
| Arduino UNO | 1 |
| LED（任意颜色） | 1~3 |
| 220Ω 电阻 | 1~3 |
| 面包板 | 1 |
| 杜邦线 | 若干 |

## 接线图

```
Arduino D13 ──── 220Ω ──── LED(+) ──── LED(-) ──── GND
```

## 实验文件
1. `LED闪烁.ino` - 基础 LED 闪烁（使用 delay）
2. `LED多频率闪烁.ino` - 进阶：使用 millis() 非阻塞控制多个 LED

## 重要函数说明

```cpp
pinMode(pin, mode);        // 设置引脚模式：OUTPUT / INPUT / INPUT_PULLUP
digitalWrite(pin, value);  // 数字输出：HIGH(1) / LOW(0)
delay(ms);                 // 阻塞延时（毫秒）
millis();                  // 返回程序运行时间（毫秒），不阻塞
```

## 思考题
1. 为什么 LED 需要串联电阻？
2. `delay()` 和 `millis()` 有什么区别？什么时候用哪个？
3. 如何让 LED 以 SOS 摩斯密码的方式闪烁？