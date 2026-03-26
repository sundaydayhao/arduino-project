# Music Audio Shield

基于 Arduino UNO R3 的音频播放 Shield，包含两个独立程序：

| 程序 | 硬件需求 | 功能 |
|------|---------|------|
| `melody_only.ino` | 仅需蜂鸣器 + 3个按键 | 5首内置旋律，无需额外模块 |
| `music_player.ino` | DFPlayer Mini + OLED + 5个按键 | SD卡MP3播放，OLED显示 |

---

## 📁 项目结构

```
shield/music-audio/
├── src/
│   ├── melody_only.ino             # 蜂鸣器旋律播放器（入门版，无需额外模块）
│   └── music_player.ino            # DFPlayer Mini MP3播放器（完整版）
├── schematic/
│   └── music_audio_schematic.json  # 嘉立创EDA 原理图（可直接导入）
└── README.md
```

---

## 🎵 程序一：melody_only.ino（蜂鸣器旋律播放器）

### 功能
- **5 首内置旋律**：生日快乐、小星星、铃儿响叮当、欢乐颂、超级马里奥主题
- **非阻塞播放**：`loop()` 不被 `delay()` 阻塞，按键响应流畅
- **速度可调**：慢（60%）/ 正常（100%）/ 快（150%）
- **LED 节拍闪烁**：随每个音符亮灭
- **串口命令控制**：选曲、播放、速度调节

### 硬件清单

| 元件 | 规格 | 数量 |
|------|------|------|
| Arduino UNO R3 | ATmega328P | 1 |
| 无源蜂鸣器 | 5V，任意频率 | 1 |
| 按键 | 6×6mm 轻触开关 | 3 |
| LED | 3mm，任意颜色 | 1 |
| 电阻 | 220Ω | 1 |
| 杜邦线 | 公对母 | 若干 |

### 接线

| 元件 | Arduino 引脚 | 说明 |
|------|-------------|------|
| 无源蜂鸣器 + | **Pin 8** | 另一端接 GND |
| 选曲按键 | **Pin 2** | 另一端接 GND（内部上拉） |
| 播放/停止按键 | **Pin 3** | 另一端接 GND（内部上拉） |
| 速度按键 | **Pin 4** | 另一端接 GND（内部上拉） |
| LED（串 220Ω） | **Pin 13** | 另一端接 GND |

> ⚠️ 必须使用**无源蜂鸣器**（passive buzzer），有源蜂鸣器只能发出固定频率，无法演奏旋律。

### 串口命令（波特率 9600）

| 命令 | 功能 |
|------|------|
| `1` ~ `5` | 选择旋律编号 |
| `P` | 播放 / 暂停 / 继续 |
| `S` | 停止 |
| `+` | 加速 +20% |
| `-` | 减速 -20% |
| `L` | 列出所有旋律 |
| `H` | 显示帮助 |

### 内置旋律列表

| 编号 | 旋律名称 | 音符数 |
|------|---------|-------|
| 1 | Happy Birthday（生日快乐） | 25 |
| 2 | Twinkle Twinkle Little Star（小星星） | 42 |
| 3 | Jingle Bells（铃儿响叮当） | 46 |
| 4 | Ode to Joy - Beethoven（欢乐颂） | 64 |
| 5 | Super Mario Bros Theme（超级马里奥） | 60 |

### 所需库
无需额外安装库，使用 Arduino 内置 `tone()` 函数。

---

## 🎶 程序二：music_player.ino（DFPlayer Mini MP3播放器）

### 功能
- **DFPlayer Mini** 播放 SD 卡中的 MP3/WAV 文件
- **OLED SSD1306** 显示曲目编号、音量进度条、播放状态动画
- **5 个按键**：播放/暂停、下一曲、上一曲、音量+、音量-
- **电位器**模拟音量控制（A0）
- **自动播放下一曲**（当前曲目结束后）
- **串口命令**控制

### 硬件清单

| 元件 | 规格 | 数量 |
|------|------|------|
| Arduino UNO R3 | ATmega328P | 1 |
| DFPlayer Mini | MP3/WAV 播放模块 | 1 |
| Micro SD 卡 | FAT32 格式，≤32GB | 1 |
| 扬声器 | 8Ω 0.5W | 1 |
| OLED 显示屏 | SSD1306，128×64，I2C | 1 |
| 按键 | 6×6mm 轻触开关 | 5 |
| 电位器 | 10kΩ | 1 |
| 电阻 | 1kΩ（DFPlayer RX保护） | 1 |
| 电阻 | 220Ω（LED限流） | 1 |
| LED | 3mm | 1 |
| 杜邦线 | 若干 | — |

### 接线

#### DFPlayer Mini

| DFPlayer 引脚 | Arduino 引脚 | 说明 |
|--------------|-------------|------|
| VCC | 5V | 电源 |
| GND | GND | 地 |
| TX | **Pin 10** | 直接连接 |
| RX | **Pin 11** → 1kΩ → DFPlayer RX | ⚠️ 必须串联 1kΩ 电阻！ |
| SPK1 | 扬声器 + | 扬声器正极 |
| SPK2 | 扬声器 - | 扬声器负极 |

#### OLED SSD1306（I2C）

| OLED 引脚 | Arduino 引脚 |
|----------|-------------|
| VCC | 3.3V 或 5V |
| GND | GND |
| SDA | **A4** |
| SCL | **A5** |

#### 按键与其他

| 元件 | Arduino 引脚 | 说明 |
|------|-------------|------|
| 播放/暂停按键 | **Pin 2** | 接 GND，内部上拉 |
| 下一曲按键 | **Pin 3** | 接 GND，内部上拉 |
| 上一曲按键 | **Pin 4** | 接 GND，内部上拉 |
| 音量+ 按键 | **Pin 5** | 接 GND，内部上拉 |
| 音量- 按键 | **Pin 6** | 接 GND，内部上拉 |
| 电位器（音量） | **A0** | 两端接 5V 和 GND |
| LED（串 220Ω） | **Pin 13** | 播放时闪烁 |

### SD 卡文件命名规则

```
SD 卡根目录：
  0001.mp3
  0002.mp3
  0003.mp3
  ...
```

> 文件名必须为 4 位数字，从 `0001` 开始。支持 MP3 和 WAV 格式。

### 串口命令（波特率 9600）

| 命令 | 功能 |
|------|------|
| `P` | 播放 / 暂停 |
| `N` | 下一曲 |
| `B` | 上一曲 |
| `S` | 停止 |
| `V15` | 设置音量为 15（范围 0–30） |
| `T` | 打印当前状态 |

### 所需库（Arduino IDE 库管理器安装）

| 库名 | 作者 | 用途 |
|------|------|------|
| **DFRobotDFPlayerMini** | DFRobot | DFPlayer Mini 驱动 |
| **U8g2** | olikraus | OLED SSD1306 驱动 |
| SoftwareSerial | Arduino 内置 | 软件串口 |

```
Arduino IDE → 工具 → 管理库
搜索 "DFRobotDFPlayerMini" → 安装
搜索 "U8g2"                → 安装（by olikraus）
```

---

## 📐 导入嘉立创EDA原理图

1. 打开 [嘉立创EDA Standard](https://lceda.cn/)
2. 菜单：**文件 → 打开 → 嘉立创EDA...**
3. 选择 `schematic/music_audio_schematic.json`

---

## 🔍 常见问题

### 蜂鸣器无声音
- 确认使用的是**无源蜂鸣器**（passive），有源蜂鸣器无法用 `tone()` 演奏旋律
- 检查 Pin 8 接线
- 用万用表测量蜂鸣器两端电阻，无源蜂鸣器约 8–16Ω

### DFPlayer 无法初始化
- 检查 RX 线是否串联了 1kΩ 电阻（必须！）
- 确认 SD 卡格式为 FAT32，文件命名为 `0001.mp3`
- DFPlayer 需要稳定的 5V 供电，建议使用外部电源

### OLED 无显示
- 检查 I2C 地址：默认 `0x3C`，部分模块为 `0x3D`
- 确认 SDA→A4，SCL→A5 接线正确

### 音量无法调节
- 电位器接线：两端分别接 5V 和 GND，中间脚接 A0
- 如果电位器和按键同时使用，电位器优先级更高

---

## 📊 引脚占用汇总

### melody_only.ino

| Arduino 引脚 | 功能 |
|-------------|------|
| Pin 2 | 选曲按键 |
| Pin 3 | 播放/停止按键 |
| Pin 4 | 速度按键 |
| Pin 8 | 无源蜂鸣器 |
| Pin 13 | 节拍 LED |

### music_player.ino

| Arduino 引脚 | 功能 |
|-------------|------|
| Pin 2 | 播放/暂停按键 |
| Pin 3 | 下一曲按键 |
| Pin 4 | 上一曲按键 |
| Pin 5 | 音量+ 按键 |
| Pin 6 | 音量- 按键 |
| Pin 10 | DFPlayer TX（SoftwareSerial RX） |
| Pin 11 | DFPlayer RX（SoftwareSerial TX，串 1kΩ） |
| Pin 13 | 状态 LED |
| A0 | 音量电位器 |
| A4 | OLED SDA（I2C） |
| A5 | OLED SCL（I2C） |

---

## 📌 版本信息

- **版本**：v1.0
- **平台**：Arduino UNO R3
- **IDE**：Arduino IDE 1.8.x / 2.x
- **DFPlayer Mini**：支持 MP3 / WAV，最大 32GB SD 卡
- **OLED**：SSD1306，128×64 像素，I2C 接口