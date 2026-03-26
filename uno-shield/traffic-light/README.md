# Traffic Light Shield（交通灯 Shield）

基于 Arduino UNO R3 的交通灯控制器，包含两个程序：基础版单路交通灯和进阶版双向交通灯。

---

## 📁 项目结构

```
shield/traffic-light/
├── src/
│   ├── traffic_light.ino           # 基础版：单路交通灯 + 行人过街
│   └── traffic_light_advanced.ino  # 进阶版：双向交通灯（待实现）
└── README.md
```

---

## ✨ 功能概览

### traffic_light.ino（基础版）

| 功能 | 说明 |
|------|------|
| 🔴 标准时序 | 红灯 → 绿灯 → 黄灯循环 |
| 🚶 行人过街 | Pin 2 按钮触发，红灯延长至 8 秒 |
| 🔔 蜂鸣器提示 | 行人过街期间每秒短鸣，切换灯时提示音 |
| 💡 绿灯闪烁 | 绿灯最后 1 秒闪烁提示即将变黄 |
| 🟡 黄灯闪烁 | 黄灯全程闪烁 |
| 📡 串口输出 | 实时打印灯态切换和行人请求信息 |

---

## 🔧 硬件清单

| 序号 | 元件 | 规格 | 数量 |
|------|------|------|------|
| 1 | Arduino UNO R3 | ATmega328P | 1 |
| 2 | LED 红色 | 5mm | 1 |
| 3 | LED 黄色 | 5mm | 1 |
| 4 | LED 绿色 | 5mm | 1 |
| 5 | 电阻 | 220Ω | 3 |
| 6 | 按键 | 6×6mm 轻触开关 | 1 |
| 7 | 无源蜂鸣器 | 5V | 1 |
| 8 | 杜邦线 | 若干 | — |

---

## 🔌 接线说明

| 元件 | Arduino 引脚 | 说明 |
|------|-------------|------|
| 红灯 LED（串 220Ω） | **Pin 13** | 另一端接 GND |
| 黄灯 LED（串 220Ω） | **Pin 12** | 另一端接 GND |
| 绿灯 LED（串 220Ω） | **Pin 11** | 另一端接 GND |
| 行人按钮 | **Pin 2** | 另一端接 GND（内部上拉，INT0） |
| 无源蜂鸣器 + | **Pin 8** | 另一端接 GND |

### 接线示意图

```
Arduino          LED（串 220Ω）
Pin 13 ──220Ω──┤A  K├── GND   红灯
Pin 12 ──220Ω──┤A  K├── GND   黄灯
Pin 11 ──220Ω──┤A  K├── GND   绿灯

Pin 2 ──────────┤SW├──── GND   行人按钮（内部上拉）

Pin 8 ──────────┤BZ├──── GND   无源蜂鸣器
```

---

## ⏱️ 时序配置

代码顶部的 `#define` 可直接修改时序：

```cpp
#define TIME_RED          5000UL   // 正常红灯时长（ms）
#define TIME_GREEN        4000UL   // 绿灯时长（ms）
#define TIME_YELLOW       2000UL   // 黄灯时长（ms）
#define TIME_PEDESTRIAN   8000UL   // 行人过街红灯时长（ms）
#define BLINK_INTERVAL     400UL   // 闪烁间隔（ms）
```

---

## 🚦 状态机说明

```
        ┌─────────────────────────────────────────┐
        │                                         │
        ▼                                         │
   ┌─────────┐   elapsed >= TIME_RED    ┌──────────────┐
   │  RED    │ ─────────────────────→  │    GREEN     │
   │ (5秒)   │                         │   (4秒)      │
   └─────────┘                         └──────────────┘
        ▲                                      │
        │                              elapsed >= TIME_GREEN
        │                                      │
        │                                      ▼
        │                              ┌──────────────┐
        │   elapsed >= TIME_YELLOW     │    YELLOW    │
        └─────────────────────────────│   (2秒闪烁)  │
                                       └──────────────┘
```

**行人过街流程：**
1. 绿灯或黄灯期间按下按钮 → 请求被记录
2. 下一个红灯开始时激活行人模式
3. 红灯延长至 `TIME_PEDESTRIAN`（8秒）
4. 期间蜂鸣器每秒短鸣提示行人通行
5. 红灯结束后恢复正常循环

---

## 📡 串口输出示例（波特率 9600）

```
=== Traffic Light Controller ===
Pin 13=RED  Pin 12=YELLOW  Pin 11=GREEN
Pin 2=Pedestrian Button  Pin 8=Buzzer
--------------------------------
[0s] → RED (5s)
[5s] → GREEN (4s)
[9s] → YELLOW (2s)
[11s] → RED (5s)
[INFO] Pedestrian request queued (will activate next red)
[INFO] Pedestrian crossing activated!
[11s] → RED (8s)   ← 行人模式，红灯延长
[19s] → GREEN (4s)
```

---

## 🚀 使用步骤

1. 按接线表连接硬件
2. 打开 `src/traffic_light.ino`
3. 选择开发板：`工具 → 开发板 → Arduino UNO`
4. 上传代码
5. 打开串口监视器（波特率 9600）观察状态输出
6. 按下 Pin 2 按钮测试行人过街功能

---

## 🔍 常见问题

### LED 不亮
- 检查 LED 方向（长脚=阳极接 Arduino，短脚=阴极接 GND）
- 确认串联了 220Ω 限流电阻
- 用万用表测量对应引脚输出电平

### 按钮无响应
- 确认按钮一端接 Pin 2，另一端接 GND
- 代码使用 `INPUT_PULLUP`，按下时为 LOW，松开时为 HIGH
- 检查中断是否正常：串口应打印 `[INFO] Pedestrian request queued`

### 蜂鸣器无声
- 确认使用**无源蜂鸣器**（passive buzzer）
- 检查 Pin 8 接线

### 时序不准确
- `millis()` 精度约 ±1ms，对交通灯场景完全足够
- 如需精确计时，可改用硬件定时器

---

## 📊 引脚占用汇总

| Arduino 引脚 | 功能 | 说明 |
|-------------|------|------|
| Pin 2 | 行人按钮 | 外部中断 INT0，下降沿触发 |
| Pin 8 | 无源蜂鸣器 | PWM 输出 |
| Pin 11 | 绿灯 LED | 数字输出 |
| Pin 12 | 黄灯 LED | 数字输出 |
| Pin 13 | 红灯 LED | 数字输出 |
| 3–7, 9–10, A0–A5 | 空闲 | 可扩展 |

---

## 💡 扩展建议

| 扩展方向 | 建议方案 |
|---------|---------|
| 双向交通灯 | 参考 `traffic_light_advanced.ino`，增加第二组 LED（Pin 5/6/7） |
| 倒计时显示 | 添加 TM1637 四位数码管，显示剩余秒数 |
| 夜间闪烁模式 | 检测光照（LDR），夜间切换为黄灯常闪模式 |
| 远程控制 | 添加蓝牙模块（HC-05），手机 App 控制灯态 |
| 多路口联动 | 通过 I2C 或 RS485 连接多个 Arduino 实现联动 |

---

## 📌 版本信息

- **版本**：v1.0
- **平台**：Arduino UNO R3
- **IDE**：Arduino IDE 1.8.x / 2.x
- **所需库**：无（使用 Arduino 内置函数）