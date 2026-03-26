# 第一章：Arduino 入门与环境搭建

> 本章目标：了解 Arduino 是什么，搭建开发环境，并运行第一个程序。

---

## 1.1 什么是 Arduino？

Arduino 是一款开源的**微控制器开发平台**，由硬件（开发板）和软件（IDE）两部分组成。它诞生于 2005 年意大利，最初是为艺术设计学生设计的，如今已成为全球最流行的创客平台之一。

### Arduino 的特点

| 特点 | 说明 |
|------|------|
| **开源免费** | 硬件设计和软件代码完全开源 |
| **易于上手** | 基于 C/C++ 的简化语法，适合初学者 |
| **生态丰富** | 数千种扩展模块和库，社区活跃 |
| **跨平台** | 支持 Windows、macOS、Linux |
| **低成本** | 官方板约 $25，兼容板更便宜 |

### Arduino 能做什么？

- 💡 控制 LED、电机、舵机等执行器
- 📡 读取温度、湿度、距离等传感器数据
- 📱 通过蓝牙、WiFi 与手机或网络通信
- 🤖 制作机器人、智能家居、自动化设备
- 🎨 创作互动艺术装置

---

## 1.2 Arduino UNO R3 详解

Arduino UNO R3 是最经典、最常用的 Arduino 开发板，也是本书的主要使用板型。

### 主要规格

| 参数 | 规格 |
|------|------|
| 微控制器 | ATmega328P |
| 工作电压 | 5V |
| 输入电压 | 7~12V（推荐） |
| 数字 I/O 引脚 | 14 个（其中 6 个支持 PWM） |
| 模拟输入引脚 | 6 个（A0~A5） |
| 每引脚最大电流 | 40mA |
| Flash 存储 | 32KB（0.5KB 用于 Bootloader） |
| SRAM | 2KB |
| EEPROM | 1KB |
| 时钟频率 | 16MHz |

### 引脚功能说明

```
Arduino UNO R3 引脚布局

数字引脚（D0~D13）：
  D0(RX), D1(TX)  → 串口通信（避免占用）
  D2, D3           → 外部中断（INT0, INT1）
  D3, D5, D6, D9, D10, D11 → PWM 输出（带 ~ 标记）
  D13              → 板载 LED

模拟引脚（A0~A5）：
  可作为模拟输入（10位ADC，0~1023）
  也可作为数字 I/O 使用

电源引脚：
  5V, 3.3V, GND, VIN
  RESET → 复位引脚

通信接口：
  I2C → A4(SDA), A5(SCL)
  SPI → D10(SS), D11(MOSI), D12(MISO), D13(SCK)
  UART → D0(RX), D1(TX)
```

### 板载组件

- **USB 接口**：用于上传程序和串口通信
- **电源插座**：DC 2.1mm 插座，接 7~12V 外部电源
- **复位按钮**：重启程序
- **板载 LED**：连接在 D13 引脚，可直接控制
- **电源 LED**：通电时常亮

---

## 1.3 安装 Arduino IDE

### 下载 Arduino IDE 2.x

1. 访问官方网站：[https://www.arduino.cc/en/software](https://www.arduino.cc/en/software)
2. 选择对应操作系统的版本下载
3. 推荐使用 **Arduino IDE 2.x**（功能更强大，支持代码补全）

### Windows 安装步骤

1. 下载 `.exe` 安装包
2. 双击运行，按提示安装
3. 安装过程中会提示安装 USB 驱动，选择**全部安装**
4. 安装完成后，桌面出现 Arduino IDE 图标

### 安装 USB 驱动

Arduino UNO 使用 **CH340** 或 **ATmega16U2** 作为 USB 转串口芯片：

- **官方 UNO**：使用 ATmega16U2，Windows 10/11 自动识别
- **兼容板（国产）**：通常使用 CH340，需手动安装驱动
  - CH340 驱动下载：[http://www.wch.cn/downloads/CH341SER_EXE.html](http://www.wch.cn/downloads/CH341SER_EXE.html)

### 验证安装

1. 用 USB 线连接 Arduino 到电脑
2. 打开**设备管理器**（Win+X → 设备管理器）
3. 在"端口（COM 和 LPT）"下应看到 `Arduino UNO (COMx)` 或 `USB-SERIAL CH340 (COMx)`

---

## 1.4 Arduino IDE 界面介绍

```
┌─────────────────────────────────────────────┐
│  文件  编辑  项目  工具  帮助                │  ← 菜单栏
├──────┬──────────────────────────────────────┤
│  ✓   │  ▶  sketch_name                      │  ← 工具栏（验证/上传）
├──────┴──────────────────────────────────────┤
│                                              │
│  void setup() {                              │
│    // 初始化代码                             │  ← 代码编辑区
│  }                                           │
│                                              │
│  void loop() {                               │
│    // 循环代码                               │
│  }                                           │
│                                              │
├──────────────────────────────────────────────┤
│  输出                                        │  ← 输出/错误信息
│  编译完成。                                  │
└──────────────────────────────────────────────┘
```

### 主要功能按钮

| 按钮 | 功能 |
|------|------|
| ✓ 验证 | 编译代码，检查语法错误 |
| → 上传 | 编译并上传到 Arduino |
| 串口监视器 | 查看串口输出数据 |
| 串口绘图仪 | 将串口数据绘制成图表 |

### 配置开发板和端口

上传程序前必须正确配置：

1. **选择开发板**：工具 → 开发板 → Arduino AVR Boards → **Arduino UNO**
2. **选择端口**：工具 → 端口 → 选择对应的 COM 口（如 COM3）

---

## 1.5 Arduino 程序结构

每个 Arduino 程序（称为 **Sketch**）都包含两个必须的函数：

```cpp
void setup() {
  // 初始化函数
  // 程序启动时只执行一次
  // 用于：设置引脚模式、初始化串口、初始化库等
}

void loop() {
  // 主循环函数
  // setup() 执行完后，loop() 会无限循环执行
  // 用于：读取传感器、控制输出、处理逻辑等
}
```

### 程序执行流程

```
上电/复位
    ↓
执行 setup()（一次）
    ↓
执行 loop()
    ↓
执行 loop()
    ↓
执行 loop()
    ↓
  ...（无限循环）
```

### 基本语法规则

```cpp
// 单行注释

/* 
   多行注释
*/

// 变量声明
int ledPin = 13;          // 整数
float voltage = 3.14;     // 浮点数
bool isOn = true;         // 布尔值
const int MAX = 255;      // 常量（不可修改）

// 条件语句
if (condition) {
  // ...
} else if (other) {
  // ...
} else {
  // ...
}

// 循环
for (int i = 0; i < 10; i++) {
  // ...
}

while (condition) {
  // ...
}

// 函数定义
int add(int a, int b) {
  return a + b;
}
```

---

## 1.6 第一个程序：Hello World（LED 闪烁）

Arduino 的 "Hello World" 就是让 LED 闪烁。Arduino UNO 的 D13 引脚连接了一个板载 LED，无需额外接线即可测试。

### 硬件连接

```
Arduino UNO
    D13 ──[220Ω]── LED(+) ── LED(-) ── GND

（也可直接使用板载 LED，无需接线）
```

### 完整代码

```cpp
/*
 * 实验名称：LED 闪烁
 * 实验目的：学习 Arduino 数字输出，控制 LED 闪烁
 * 
 * 硬件连接：
 *   LED 正极 → 220Ω 电阻 → Arduino D13
 *   LED 负极 → Arduino GND
 * 
 * 知识点：
 *   - pinMode()      设置引脚模式
 *   - digitalWrite() 数字输出
 *   - delay()        延时函数
 */

// 定义 LED 连接的引脚
const int LED_PIN = 13;

void setup() {
  // 将 LED 引脚设置为输出模式
  pinMode(LED_PIN, OUTPUT);
  
  // 初始化串口，波特率 9600
  Serial.begin(9600);
  Serial.println("LED 闪烁实验开始！");
}

void loop() {
  // 点亮 LED
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED 亮");
  delay(1000);  // 等待 1 秒
  
  // 熄灭 LED
  digitalWrite(LED_PIN, LOW);
  Serial.println("LED 灭");
  delay(1000);  // 等待 1 秒
}
```

### 上传步骤

1. 将代码复制到 Arduino IDE
2. 点击 **✓ 验证**，确认无错误
3. 用 USB 线连接 Arduino
4. 选择正确的**开发板**和**端口**
5. 点击 **→ 上传**
6. 等待上传完成（底部显示"上传完成"）
7. 观察 D13 旁边的 LED 开始闪烁

### 查看串口输出

1. 点击右上角的**串口监视器**图标（或 Ctrl+Shift+M）
2. 设置波特率为 **9600**
3. 可以看到 "LED 亮" 和 "LED 灭" 交替输出

---

## 1.7 常用 Arduino 函数速览

### 数字 I/O

```cpp
pinMode(pin, mode);           // 设置引脚模式
                              // mode: INPUT, OUTPUT, INPUT_PULLUP

digitalWrite(pin, value);     // 数字输出
                              // value: HIGH(1) 或 LOW(0)

int val = digitalRead(pin);   // 数字输入，返回 HIGH 或 LOW
```

### 模拟 I/O

```cpp
analogWrite(pin, value);      // PWM 输出（0~255）
                              // 仅 PWM 引脚：3,5,6,9,10,11

int val = analogRead(pin);    // 模拟输入（0~1023）
                              // 仅模拟引脚：A0~A5
```

### 时间

```cpp
delay(ms);                    // 延时（毫秒），阻塞式
delayMicroseconds(us);        // 延时（微秒）
unsigned long t = millis();   // 返回程序运行毫秒数
unsigned long t = micros();   // 返回程序运行微秒数
```

### 串口通信

```cpp
Serial.begin(baudRate);       // 初始化串口（常用 9600）
Serial.print(data);           // 输出数据（不换行）
Serial.println(data);         // 输出数据（换行）
int n = Serial.available();   // 检查接收缓冲区字节数
int b = Serial.read();        // 读取一个字节
```

### 数学运算

```cpp
int m = map(val, fromLow, fromHigh, toLow, toHigh);  // 数值映射
int a = abs(x);               // 绝对值
int mn = min(a, b);           // 最小值
int mx = max(a, b);           // 最大值
int c = constrain(x, a, b);  // 限制在 [a,b] 范围内
float r = sqrt(x);            // 平方根
```

---

## 1.8 安装第三方库

许多传感器和模块需要安装第三方库才能使用。

### 通过库管理器安装（推荐）

1. 打开 Arduino IDE
2. 菜单：**工具 → 管理库**（或 Ctrl+Shift+I）
3. 在搜索框输入库名称
4. 找到对应库，点击**安装**

### 常用库列表

| 库名称 | 用途 | 安装名称 |
|--------|------|----------|
| DHT sensor library | DHT11/DHT22 温湿度 | `DHT sensor library` |
| Adafruit SSD1306 | OLED 显示屏 | `Adafruit SSD1306` |
| LiquidCrystal I2C | LCD1602 I2C | `LiquidCrystal I2C` |
| IRremote | 红外遥控 | `IRremote` |
| Servo | 舵机控制 | 内置，无需安装 |
| MFRC522 | RFID RC522 | `MFRC522` |
| RTClib | DS3231 RTC | `RTClib` |
| LedControl | MAX7219 点阵 | `LedControl` |

### 手动安装库

1. 下载库的 `.zip` 文件
2. Arduino IDE → **项目 → 加载库 → 添加 .ZIP 库**
3. 选择下载的 zip 文件

---

## 1.9 本章小结

✅ 了解了 Arduino 的特点和应用场景  
✅ 掌握了 Arduino UNO R3 的引脚功能  
✅ 安装并配置了 Arduino IDE  
✅ 理解了 Arduino 程序的基本结构  
✅ 成功运行了第一个 LED 闪烁程序  
✅ 学会了安装第三方库  

### 思考题

1. Arduino UNO 有几个 PWM 引脚？分别是哪些？
2. `setup()` 和 `loop()` 函数的区别是什么？
3. 如果要让 LED 每 0.5 秒闪烁一次，`delay()` 的参数应该是多少？

---

[← 返回目录](./README.md) | [下一章：基础控制实验 →](./第二章_基础控制实验.md)