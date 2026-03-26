# JMISC PSSI 接口连接 OV7725 FIFO 摄像头模块接线说明

本文档说明如何将 **ALIENTEK OV7725 V1.1（带 AL422B FIFO 缓冲）** 并行摄像头模块连接到 Arduino Uno Q 扩展板的 **JMISC PSSI 接口**。

---

## 一、模块与接口简介

### OV7725 FIFO 模块引脚

| 引脚名 | 方向 | 说明 |
|--------|------|------|
| D0 ~ D7 | OUT | 8 位并行图像数据（从 FIFO 读出） |
| RCLK | IN | FIFO 读时钟（由 MCU 提供） |
| RRST | IN | FIFO 读指针复位（低有效） |
| OE | IN | FIFO 输出使能（低有效） |
| WRST | IN | FIFO 写指针复位（低有效） |
| WEN | — | FIFO 写使能（模块内部已连接 HREF，无需外接） |
| VSYNC | OUT | 帧同步信号（来自 OV7725） |
| SCL | IN | SCCB 配置时钟（I2C 兼容） |
| SDA | IN/OUT | SCCB 配置数据（I2C 兼容） |
| 3.3V | IN | 3.3V 电源 |
| GND | — | 地 |

### JMISC PSSI 接口信号

| JMISC 引脚 | 信号名 | GPIO | 说明 |
|-----------|--------|------|------|
| 1 | MCU_PSSI_D0 | PC6 | 数据位 0 |
| 3 | MCU_PSSI_D1 | PC7 | 数据位 1 |
| 5 | MCU_PSSI_D2 | PC8 | 数据位 2 |
| 7 | MCU_PSSI_D3 | PC9 | 数据位 3 |
| 9 | MCU_PSSI_D4 | PE4 | 数据位 4 |
| 11 | MCU_PSSI_D5 | PI4 | 数据位 5 |
| 13 | MCU_PSSI_D6 | PI6 | 数据位 6 |
| 15 | MCU_PSSI_D7 | PI7 | 数据位 7 |
| 17 | MCU_PSSI_PDCK | PD9 | 像素时钟（主模式下 MCU 输出） |
| 19 | MCU_PSSI_RDY | PI5 | 就绪信号（接 VSYNC） |
| 21 | MCU_PSSI_DE | PD8 | 数据使能（接 OE） |
| 16 | MCU_I2C4_SCL | PF14 | I2C4 时钟（配置 OV7725 寄存器） |
| 18 | MCU_I2C4_SDA | PF15 | I2C4 数据（配置 OV7725 寄存器） |
| 23 | MCU_MCO | PA8 | MCU 时钟输出（可选，提供 XCLK） |
| 53 / 55 | +3V3 | — | 3.3V 电源输出 |
| 27 / 35 | GND | — | 地 |

---

## 二、完整接线表

| OV7725 引脚 | JMISC 引脚号 | JMISC 信号名 | GPIO | 说明 |
|------------|:-----------:|-------------|------|------|
| **D0** | 1 | MCU_PSSI_D0 | PC6 | 数据位 0 |
| **D1** | 3 | MCU_PSSI_D1 | PC7 | 数据位 1 |
| **D2** | 5 | MCU_PSSI_D2 | PC8 | 数据位 2 |
| **D3** | 7 | MCU_PSSI_D3 | PC9 | 数据位 3 |
| **D4** | 9 | MCU_PSSI_D4 | PE4 | 数据位 4 |
| **D5** | 11 | MCU_PSSI_D5 | PI4 | 数据位 5 |
| **D6** | 13 | MCU_PSSI_D6 | PI6 | 数据位 6 |
| **D7** | 15 | MCU_PSSI_D7 | PI7 | 数据位 7 |
| **RCLK** | 17 | MCU_PSSI_PDCK | PD9 | FIFO 读时钟（PSSI 主模式输出） |
| **VSYNC** | 19 | MCU_PSSI_RDY | PI5 | 帧同步（触发一帧读取开始） |
| **OE** | 21 | MCU_PSSI_DE | PD8 | FIFO 输出使能（低有效） |
| **SCL** | 16 | MCU_I2C4_SCL | PF14 | SCCB 配置时钟 |
| **SDA** | 18 | MCU_I2C4_SDA | PF15 | SCCB 配置数据 |
| **WRST** | 任意 MCU GPIO | — | 自选 | FIFO 写指针复位（低有效） |
| **RRST** | 任意 MCU GPIO | — | 自选 | FIFO 读指针复位（低有效） |
| **WEN** | 悬空 | — | — | 模块内部已连接 HREF，无需外接 |
| **3.3V** | 53 或 55 | +3V3 | — | 3.3V 电源 |
| **GND** | 27 或 35 | GND | — | 地 |

---

## 三、连接示意图

```
OV7725 FIFO 模块              JMISC 接口（60-pin）
─────────────────             ──────────────────────────────
D0  ──────────────────────→   Pin 1   MCU_PSSI_D0  (PC6)
D1  ──────────────────────→   Pin 3   MCU_PSSI_D1  (PC7)
D2  ──────────────────────→   Pin 5   MCU_PSSI_D2  (PC8)
D3  ──────────────────────→   Pin 7   MCU_PSSI_D3  (PC9)
D4  ──────────────────────→   Pin 9   MCU_PSSI_D4  (PE4)
D5  ──────────────────────→   Pin 11  MCU_PSSI_D5  (PI4)
D6  ──────────────────────→   Pin 13  MCU_PSSI_D6  (PI6)
D7  ──────────────────────→   Pin 15  MCU_PSSI_D7  (PI7)
RCLK ←────────────────────    Pin 17  MCU_PSSI_PDCK (PD9)  ← MCU 输出读时钟
VSYNC ────────────────────→   Pin 19  MCU_PSSI_RDY  (PI5)
OE   ─────────────────────→   Pin 21  MCU_PSSI_DE   (PD8)
SCL  ─────────────────────→   Pin 16  MCU_I2C4_SCL  (PF14)
SDA  ─────────────────────→   Pin 18  MCU_I2C4_SDA  (PF15)
WRST ─────────────────────→   MCU GPIO（自选，复位 FIFO 写指针）
RRST ─────────────────────→   MCU GPIO（自选，复位 FIFO 读指针）
3.3V ─────────────────────→   Pin 53  +3V3
GND  ─────────────────────→   Pin 27  GND
```

---

## 四、FIFO 读取时序说明

OV7725 FIFO 模块的工作分为两个阶段：**写入阶段**（OV7725 → FIFO）和**读取阶段**（FIFO → MCU）。

```
① VSYNC 上升沿
   └─ 一帧图像开始写入 FIFO

② VSYNC 下降沿
   └─ 一帧图像写入完成

③ MCU 拉低 WRST（写指针复位）→ 再拉高
   └─ 准备接收下一帧（可选，用于连续采集）

④ MCU 拉低 RRST（读指针复位）→ 再拉高
   └─ FIFO 读指针回到帧头

⑤ MCU 拉低 OE（或由 PSSI_DE 控制）
   └─ 使能 FIFO 数据输出

⑥ PSSI 输出 RCLK（通过 PDCK 引脚）
   └─ 每个时钟上升沿读取一字节图像数据
   └─ 连续读取直到整帧数据读完
```

---

## 五、MCU 软件配置要点

### 5.1 PSSI 配置（主模式）

```c
// STM32U585 PSSI 主模式配置（HAL 库）
PSSI_HandleTypeDef hpssi;

hpssi.Instance              = PSSI;
hpssi.Init.BusWidth         = PSSI_8LINES;          // 8 位数据线
hpssi.Init.ControlSignal    = PSSI_RDY_ENABLE;       // 使能 RDY 信号（接 VSYNC）
hpssi.Init.ClockPolarity    = PSSI_FALLING_EDGE;     // 下降沿采样
hpssi.Init.DataLane         = PSSI_ALL_DATA_LINES;   // 全部 8 位数据线
HAL_PSSI_Init(&hpssi);
```

### 5.2 OV7725 SCCB 配置

```c
// OV7725 I2C 地址
#define OV7725_WRITE_ADDR  0x42
#define OV7725_READ_ADDR   0x43

// 通过 MCU I2C4（PF14/PF15）写寄存器
void OV7725_WriteReg(uint8_t reg, uint8_t val) {
    HAL_I2C_Mem_Write(&hi2c4, OV7725_WRITE_ADDR, reg,
                      I2C_MEMADD_SIZE_8BIT, &val, 1, 100);
}

// 推荐初始化：QVGA（320×240）RGB565 格式
OV7725_WriteReg(0x12, 0x40);  // COM7: QVGA + RGB 输出
OV7725_WriteReg(0x3D, 0x03);  // COM12: RGB565
```

### 5.3 FIFO 读取流程

```c
// WRST / RRST 引脚定义（根据实际 GPIO 自选）
#define WRST_PIN  GPIO_PIN_X   // 自选 MCU GPIO
#define RRST_PIN  GPIO_PIN_Y   // 自选 MCU GPIO

void Camera_CaptureFrame(uint8_t *buf, uint32_t size) {
    // 1. 等待 VSYNC 下降沿（一帧写入完成）
    while (HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_5) == GPIO_PIN_SET);
    while (HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_5) == GPIO_PIN_RESET);

    // 2. 复位 FIFO 读指针
    HAL_GPIO_WritePin(RRST_PORT, RRST_PIN, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(RRST_PORT, RRST_PIN, GPIO_PIN_SET);

    // 3. 通过 PSSI 读取整帧数据
    HAL_PSSI_Receive(&hpssi, buf, size, 1000);
}
```

---

## 六、关键注意事项

1. **PSSI 工作在主模式**：RCLK 由 MCU 的 PSSI_PDCK（PD9）输出，建议频率 **8 ~ 24 MHz**，控制 FIFO 读取速度。

2. **OV7725 需要 XCLK 时钟**：OV7725 摄像头本身需要外部时钟（XCLK，通常 24 MHz）。可使用 JMISC 的 **MCU_MCO（PA8，引脚 23）** 输出，也可使用独立晶振模块提供。

3. **推荐分辨率**：
   | 分辨率 | 格式 | 单帧大小 | 说明 |
   |--------|------|----------|------|
   | 320×240 QVGA | RGB565 | 150 KB | ✅ 推荐，SRAM 充裕 |
   | 640×480 VGA | RGB565 | 600 KB | ⚠️ 接近 SRAM 上限 |
   | 160×120 QQVGA | RGB565 | 37 KB | ✅ 适合 AI 推理 |

4. **OV7725 SCCB 地址**：写地址 `0x42`，读地址 `0x43`，通过 JMISC 的 MCU_I2C4（PF14/PF15）配置。

5. **WEN 引脚**：ALIENTEK OV7725 V1.1 模块的 WEN 已在模块内部连接到 HREF，无需外部连接，悬空即可。

6. **OE 极性**：OE 为低有效，PSSI_DE（PD8）输出低电平时使能 FIFO 数据输出。若不使用 PSSI_DE 控制，可将 OE 直接接 GND（始终使能）。

---

## 七、与 J6 板载摄像头接口的区别

| 对比项 | J6（板载 Arducam 接口） | JMISC PSSI 接口 |
|--------|----------------------|----------------|
| MCU 外设 | DCMI | PSSI |
| 数据线 GPIO | PH9/PH10/PH11/PG11/PH14/PI4/PI6/PI7 | PC6/PC7/PC8/PC9/PE4/PI4/PI6/PI7 |
| 像素时钟 GPIO | PA6 | PD9 |
| 行同步 GPIO | PH8 (HREF) | PD8 (DE) |
| I2C 配置 GPIO | PB6 / PH12 | PF14 / PF15 (I2C4) |
| 适用摄像头 | Arducam 系列（OV2640/OV5642 等） | 通用并行摄像头（OV7725/OV7670/MT9V034 等） |
| 两者能否共用 | ❌ GPIO 不同，不能互换 | ❌ GPIO 不同，不能互换 |

> **注意**：J6 和 JMISC PSSI 是两套独立的摄像头接口，使用不同的 MCU 外设和 GPIO，不能互相替代，也不能同时驱动同一个摄像头模块。