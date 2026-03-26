# 七段数码管实验

## 模块简介
七段数码管通过控制 7 个 LED 段（a~g）和小数点（dp）来显示数字和部分字母。

## 7段编码

```
 _
|_|  ← 段 a(上), b(右上), c(右下), d(下), e(左下), f(左上), g(中)
|_|

数字编码（共阴极）：
0: abcdef  = 0x3F
1: bc      = 0x06
2: abdeg   = 0x5B
3: abcdg   = 0x4F
4: bcfg    = 0x66
5: acdfg   = 0x6D
6: acdefg  = 0x7D
7: abc     = 0x07
8: abcdefg = 0x7F
9: abcdfg  = 0x6F
```

## TM1637 模块连接

```
TM1637        Arduino
CLK ────────── D2
DIO ────────── D3
VCC ────────── 5V
GND ────────── GND
```

## 需要安装的库
- **TM1637Display**：`工具 → 管理库 → 搜索 TM1637`

## 实验文件
1. `数码管显示.ino` - 显示数字、计数、时间格式

## 常用函数

```cpp
display.setBrightness(n);           // 亮度 0~7
display.showNumberDec(num, zeros);  // 显示十进制数
display.showNumberDecEx(num, dots, zeros); // 带小数点/冒号
display.setSegments(data);          // 直接设置段码
display.clear();                    // 清除显示
```

## 思考题
1. 动态扫描是什么原理？为什么人眼看不出闪烁？
2. 如何用数码管显示字母？哪些字母可以显示？
3. 如何实现一个倒计时计时器？