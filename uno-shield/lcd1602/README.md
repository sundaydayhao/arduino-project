# Arduino UNO + LCD1602 时钟

这个项目当前不使用 RTC 硬件。

LCD1602 与 Arduino 之间使用并行接口，时间由程序内部的软件时钟维护。  
软件时钟基于 `millis()` 运行，Arduino 复位或断电后时间不会保留。

## 项目结构

```text
lcd1602/
├─ src/
│  ├─ arduino_clock/
│  │  └─ arduino_clock.ino
│  └─ lcd_test/
│     └─ lcd_test.ino
└─ README.md
```

## 硬件清单

| 元件 | 说明 |
|------|------|
| Arduino UNO R3 | 主控 |
| LCD1602 | 16x2 字符液晶，HD44780 兼容，并行接口 |
| 10K 电位器 | LCD 对比度调节 |
| 杜邦线 | 若干 |

## 接线说明

本项目代码采用 4 位并行模式，当前引脚定义如下：

| LCD1602 引脚 | 连接到 Arduino UNO | 说明 |
|-------------|---------------------|------|
| VSS | GND | 地 |
| VDD | 5V | 电源 |
| VO | 10K 电位器中间脚 | 对比度调节 |
| RS | D8 | 寄存器选择 |
| RW | GND | 固定写模式 |
| E | D9 | 使能 |
| D4 | D4 | 数据线 |
| D5 | D5 | 数据线 |
| D6 | D6 | 数据线 |
| D7 | D7 | 数据线 |
| A / LED+ | 5V | 背光正极 |
| K / LED- | GND | 背光负极 |

10K 电位器推荐接法：

| 电位器引脚 | 连接 |
|-----------|------|
| 一侧 | 5V |
| 中间 | LCD `VO` |
| 另一侧 | GND |

如果你的 LCD 实际接线不同，直接修改 [`src/arduino_clock/arduino_clock.ino`](D:\workspace\arduino-project\uno-shield\lcd1602\src\arduino_clock\arduino_clock.ino#L42) 顶部这些宏：

```cpp
#define LCD_RS 8
#define LCD_EN 9
#define LCD_D4 4
#define LCD_D5 5
#define LCD_D6 6
#define LCD_D7 7
```

## 依赖库

需要 `LiquidCrystal`。这个库通常是 Arduino IDE 自带库，不需要额外安装。

## 功能

- LCD 第一行显示日期和星期，例如 `2026-03-28 Sat`
- LCD 第二行显示时间和状态，例如 `21:30:45 No RTC`
- 上电后默认从编译时间开始计时
- 支持串口命令读取和设置软件时钟

## 串口命令

波特率：`9600`

```text
R
```

读取当前软件时钟时间。

```text
T2024-03-25 12:30:00
```

将软件时钟设置为指定时间。

## 上传说明

1. 用 Arduino IDE 打开 [`src/arduino_clock/arduino_clock.ino`](D:\workspace\arduino-project\uno-shield\lcd1602\src\arduino_clock\arduino_clock.ino)
2. 开发板选择 `Arduino UNO`
3. 选择正确的串口
4. 编译并上传

## LCD 自检

如果串口有输出，但 LCD 只有背光没有字符，先上传 [`src/lcd_test/lcd_test.ino`](D:\workspace\arduino-project\uno-shield\lcd1602\src\lcd_test\lcd_test.ino)。

这个草图只做一件事：反复在 LCD 上显示固定文本，不涉及时钟逻辑。如果这个测试也没有字符显示，问题基本就在接线或对比度，而不是主程序。

## 限制

- 这不是高精度时钟，时间精度取决于 Arduino 自身时基
- 复位、断电、重新上电后，时间会回到重新编译时的时间，或者你串口手动设置的时间会丢失
- 当前版本不显示 RTC 温度，因为没有 DS3231

## 常见问题

### LCD 不显示

- 检查 `VSS/VDD` 和背光供电是否正确
- 检查 `RW` 是否已经接地
- 调整 10K 电位器，避免对比度过低
- 确认 `RS/E/D4/D5/D6/D7` 与代码中的宏一致

### 时间不准

- 软件时钟本身不适合长期精确计时
- 通过串口发送 `TYYYY-MM-DD HH:MM:SS` 重新设定时间
- 如果后续需要掉电保持和更高精度，再接入 DS3231
