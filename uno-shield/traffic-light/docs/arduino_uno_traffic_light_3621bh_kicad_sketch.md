# Arduino Uno + 4x3621BH 十字路口红绿灯 KiCad 风格原理图草图

本文不是正式 `.kicad_sch` 文件，而是一份可直接照着在 `KiCad` 中落图的原理图草图。

对应的实际 `KiCad` 层级原理图文件已经生成：

- 顶层文件：[kicad/traffic_light_uno_3621bh.kicad_sch](/D:/workspace/arduino-project/uno-shield/traffic-light/kicad/traffic_light_uno_3621bh.kicad_sch)
- 子页文件：[kicad/power_and_ctrl.kicad_sch](/D:/workspace/arduino-project/uno-shield/traffic-light/kicad/power_and_ctrl.kicad_sch)
- 子页文件：[kicad/traffic_lamps_and_buzzer.kicad_sch](/D:/workspace/arduino-project/uno-shield/traffic-light/kicad/traffic_lamps_and_buzzer.kicad_sch)
- 子页文件：[kicad/display_segments.kicad_sch](/D:/workspace/arduino-project/uno-shield/traffic-light/kicad/display_segments.kicad_sch)
- 子页文件：[kicad/display_digits.kicad_sch](/D:/workspace/arduino-project/uno-shield/traffic-light/kicad/display_digits.kicad_sch)

适用方案：

- `Arduino Uno`
- `4 x 74HC595`
- `3 x ULN2803A`
- `8 x BC327`
- `4 x 3621BH` 共阳两位数码管
- `12` 路红黄绿 LED
- `1` 路 `5V` 有源蜂鸣器

## 1. 元件位号建议

| 位号 | 器件 | 说明 |
| --- | --- | --- |
| `A1` | `Arduino_Uno_R3` | 主控板 |
| `U1` | `74HC595` | `SR1`，交通灯前 8 路 |
| `U2` | `74HC595` | `SR2`，交通灯后 4 路 + 蜂鸣器 + 预留 |
| `U3` | `74HC595` | `SR3`，数码管段线 |
| `U4` | `74HC595` | `SR4`，数码管位选 |
| `U5` | `ULN2803A` | `LAMP_DRV_A` |
| `U6` | `ULN2803A` | `LAMP_DRV_B` |
| `U7` | `ULN2803A` | `SEG_DRV` |
| `Q1-Q8` | `BC327` | 8 个数位高边位选 |
| `DS1` | `3621BH` | North |
| `DS2` | `3621BH` | South |
| `DS3` | `3621BH` | East |
| `DS4` | `3621BH` | West |
| `BZ1` | `Buzzer_Active` | `5V` 有源蜂鸣器 |
| `D1-D12` | `LED` | 12 路交通灯 LED |
| `R1-R12` | `220R/330R` | 交通灯限流电阻 |
| `R13-R20` | `220R/330R` | 数码管段限流电阻 |
| `R21-R28` | `1k` | `BC327` 基极串联电阻 |
| `R29-R36` | `10k` | `BC327` 基极上拉电阻 |
| `C1-C7` | `100nF` | `U1-U7` 去耦 |
| `C8-C9` | `47uF-220uF` | 电源滤波 |

## 2. 网络名建议

### 2.1 电源网络

- `+5V`
- `GND`

### 2.2 Uno 到移位寄存器

- `SR_DATA`
- `SR_CLK`
- `SR_LATCH`

### 2.3 移位寄存器级联

- `SR1_Q7S`
- `SR2_Q7S`
- `SR3_Q7S`

### 2.4 交通灯网络

- `NR`, `NY`, `NG`
- `SR`, `SY`, `SG`
- `ER`, `EY`, `EG`
- `WR`, `WY`, `WG`
- `BUZZER_EN`

### 2.5 数码管段线网络

- `SEG_A`
- `SEG_B`
- `SEG_C`
- `SEG_D`
- `SEG_E`
- `SEG_F`
- `SEG_G`
- `SEG_DP`

### 2.6 数码管位选网络

- `DIG_N_TEN`
- `DIG_N_ONE`
- `DIG_S_TEN`
- `DIG_S_ONE`
- `DIG_E_TEN`
- `DIG_E_ONE`
- `DIG_W_TEN`
- `DIG_W_ONE`

## 3. 总体结构草图

```text
            +--------------------+
            |   Arduino Uno A1   |
            |                    |
 D2 --------+ SR_DATA            |
 D3 --------+ SR_CLK             |
 D4 --------+ SR_LATCH           |
 5V --------+ +5V                |
 GND -------+ GND                |
            +---------+----------+
                      |
                      v
    +---------+   +---------+   +---------+   +---------+
    | U1      |-->| U2      |-->| U3      |-->| U4      |
    | 74HC595 |   | 74HC595 |   | 74HC595 |   | 74HC595 |
    | lamps A |   | lamps B |   | segments|   | digits  |
    +----+----+   +----+----+   +----+----+   +----+----+
         |             |             |             |
         v             v             v             v
      +-----+       +-----+       +-----+     8 x BC327
      | U5  |       | U6  |       | U7  |         |
      |ULN  |       |ULN  |       |ULN  |         |
      +--+--+       +--+--+       +--+--+         |
         |             |             |             |
     8 路灯负极     4 路灯负极+蜂鸣器   8 路段线下拉      8 路位选高边
                                                   |
                                              +----+----+
                                              | DS1-DS4 |
                                              | 3621BH  |
                                              +---------+
```

## 4. 电源页草图

```text
                         +5V
                          |
          +---------------+-----------------------------+
          |               |               |             |
         C8              C9              A1            BZ1(+)
     47uF~220uF      47uF~220uF         +5V             |
          |               |               |             |
         GND             GND             GND           BZ1(-) --> U6 OUT5

+5V also connects to:
- U1/U2/U3/U4 pin16
- U5/U6/U7 pin9
- Q1-Q8 emitters
- R29-R36 upper ends
- DS1/DS2/DS3/DS4 digit commons through Q1-Q8 high-side switches
```

去耦建议：

- `C1-C4` 分别放在 `U1-U4` 的 `VCC` 和 `GND` 之间
- `C5-C7` 分别放在 `U5-U7` 的 `VCC` 和 `GND` 之间
- `C8/C9` 放在数码管和交通灯电源入口附近

## 5. Arduino Uno 与 74HC595 连接

### 5.1 控制线

```text
A1 D2  -> SR_DATA  -> U1 pin14 DS
A1 D3  -> SR_CLK   -> U1/U2/U3/U4 pin11 SHCP
A1 D4  -> SR_LATCH -> U1/U2/U3/U4 pin12 STCP
```

### 5.2 级联

```text
U1 pin9  Q7S -> U2 pin14 DS
U2 pin9  Q7S -> U3 pin14 DS
U3 pin9  Q7S -> U4 pin14 DS
```

### 5.3 固定脚

```text
U1/U2/U3/U4 pin13 OE  -> GND
U1/U2/U3/U4 pin10 MR  -> +5V
U1/U2/U3/U4 pin16 VCC -> +5V
U1/U2/U3/U4 pin8  GND -> GND
```

## 6. `U1/U2` 交通灯与蜂鸣器页草图

### 6.1 `U1` 到 `U5`

| `U1` 输出 | 网络名 | `U5` 输入 | `U5` 输出 | 负载 |
| --- | --- | --- | --- | --- |
| `Q0` | `NR` | `IN1` | `OUT1` | 北红灯阴极 |
| `Q1` | `NY` | `IN2` | `OUT2` | 北黄灯阴极 |
| `Q2` | `NG` | `IN3` | `OUT3` | 北绿灯阴极 |
| `Q3` | `SR` | `IN4` | `OUT4` | 南红灯阴极 |
| `Q4` | `SY` | `IN5` | `OUT5` | 南黄灯阴极 |
| `Q5` | `SG` | `IN6` | `OUT6` | 南绿灯阴极 |
| `Q6` | `ER` | `IN7` | `OUT7` | 东红灯阴极 |
| `Q7` | `EY` | `IN8` | `OUT8` | 东黄灯阴极 |

### 6.2 `U2` 到 `U6`

| `U2` 输出 | 网络名 | `U6` 输入 | `U6` 输出 | 负载 |
| --- | --- | --- | --- | --- |
| `Q0` | `EG` | `IN1` | `OUT1` | 东绿灯阴极 |
| `Q1` | `WR` | `IN2` | `OUT2` | 西红灯阴极 |
| `Q2` | `WY` | `IN3` | `OUT3` | 西黄灯阴极 |
| `Q3` | `WG` | `IN4` | `OUT4` | 西绿灯阴极 |
| `Q4` | `BUZZER_EN` | `IN5` | `OUT5` | 蜂鸣器负极 |
| `Q5` | `RESERVED1` | `IN6` | `OUT6` | 预留 |
| `Q6` | `RESERVED2` | `IN7` | `OUT7` | 预留 |
| `Q7` | `RESERVED3` | `IN8` | `OUT8` | 预留 |

### 6.3 单路 LED 草图

```text
+5V -> R1 -> D1(anode)
D1(cathode) -> U5 OUT1
U5 IN1 <- NR
```

其余 11 路 LED 同理。

### 6.4 蜂鸣器草图

```text
+5V ---------> BZ1(+)
BZ1(-) ------> U6 OUT5
U6 IN5 <------ BUZZER_EN
U6 GND -------> GND
U6 COM pin10 -> NC
```

说明：

- 默认采用 `5V` 有源蜂鸣器
- `BUZZER_EN` 为高时蜂鸣器响
- 若改无源蜂鸣器，需要改用 `Arduino PWM + 三极管`，不建议继续挂在 `74HC595`

## 7. `U3/U7` 数码管段线页草图

### 7.1 `U3` 到 `U7`

| `U3` 输出 | 网络名 | `U7` 输入 | `U7` 输出 | 串联电阻 | 段总线 |
| --- | --- | --- | --- | --- | --- |
| `Q0` | `SEG_A_DRV` | `IN1` | `OUT1` | `R13` | `SEG_A` |
| `Q1` | `SEG_B_DRV` | `IN2` | `OUT2` | `R14` | `SEG_B` |
| `Q2` | `SEG_C_DRV` | `IN3` | `OUT3` | `R15` | `SEG_C` |
| `Q3` | `SEG_D_DRV` | `IN4` | `OUT4` | `R16` | `SEG_D` |
| `Q4` | `SEG_E_DRV` | `IN5` | `OUT5` | `R17` | `SEG_E` |
| `Q5` | `SEG_F_DRV` | `IN6` | `OUT6` | `R18` | `SEG_F` |
| `Q6` | `SEG_G_DRV` | `IN7` | `OUT7` | `R19` | `SEG_G` |
| `Q7` | `SEG_DP_DRV` | `IN8` | `OUT8` | `R20` | `SEG_DP` |

### 7.2 单段草图

```text
DS1 pin3 ----+
DS2 pin3 ----+---- SEG_A ---- R13 ---- U7 OUT1
DS3 pin3 ----+
DS4 pin3 ----+

U7 IN1 <------ U3 Q0
```

其他段 `B/C/D/E/F/G/DP` 完全同理。

### 7.3 3621BH 段脚统一定义

| 段 | 引脚 |
| --- | --- |
| `A` | `pin3` |
| `B` | `pin9` |
| `C` | `pin8` |
| `D` | `pin6` |
| `E` | `pin7` |
| `F` | `pin4` |
| `G` | `pin1` |
| `DP` | `pin2` |

## 8. `U4 + Q1-Q8` 数码管位选页草图

### 8.1 单个高边位选草图

```text
              +5V
               |
               E
             Q1 BC327
               C ------------------> DIG_N_TEN
               B
               |
U4 Q0 ---- R21 1k
               |
             R29 10k
               |
              +5V
```

逻辑关系：

- `U4 Q0 = LOW` 时，`Q1` 导通，`DIG_N_TEN` 得到 `+5V`
- `U4 Q0 = HIGH` 时，`Q1` 截止

### 8.2 位选分配

| `U4` 输出 | 三极管 | 位选网络 | 数码管引脚 |
| --- | --- | --- | --- |
| `Q0` | `Q1` | `DIG_N_TEN` | `DS1 pin10` |
| `Q1` | `Q2` | `DIG_N_ONE` | `DS1 pin5` |
| `Q2` | `Q3` | `DIG_S_TEN` | `DS2 pin10` |
| `Q3` | `Q4` | `DIG_S_ONE` | `DS2 pin5` |
| `Q4` | `Q5` | `DIG_E_TEN` | `DS3 pin10` |
| `Q5` | `Q6` | `DIG_E_ONE` | `DS3 pin5` |
| `Q6` | `Q7` | `DIG_W_TEN` | `DS4 pin10` |
| `Q7` | `Q8` | `DIG_W_ONE` | `DS4 pin5` |

### 8.3 3621BH 位脚定义

| 位脚 | 引脚 |
| --- | --- |
| `DIG1` | `pin10` |
| `DIG2` | `pin5` |

## 9. 适合在 KiCad 中分成的原理图页

建议按 4 张子页来画：

1. `power_and_ctrl.kicad_sch`
   - `Arduino Uno`
   - `U1-U4`
   - 电源、去耦、总控制线
2. `traffic_lamps_and_buzzer.kicad_sch`
   - `U5-U6`
   - `D1-D12`
   - `BZ1`
3. `display_segments.kicad_sch`
   - `U7`
   - `R13-R20`
   - `SEG_A` 到 `SEG_DP`
4. `display_digits.kicad_sch`
   - `Q1-Q8`
   - `R21-R36`
   - `DS1-DS4`

如果你只画单页图，也建议用全局网络名，不要满图拉长线。

## 10. KiCad 落图时的注意点

- `74HC595` 使用同一个符号库版本，保持引脚编号一致
- `ULN2803A` 的 `COM` 在本项目里可以悬空，不需要接到 `+5V`
- `BC327` 的 `E/B/C` 排列必须按你实际库符号和手上器件数据手册核对
- `3621BH` 如果库里没有现成符号，建议自建一个包含 `A-G/DP/DIG1/DIG2` 的简单符号
- 段线和位选线要明确标注“低有效”或“高边开关”，避免后续软件逻辑写反

## 11. 后续可继续补的内容

如果你要继续往下做，我可以直接补其中一种：

- 正式 `.kicad_sch` 结构草案
- `KiCad` 原理图页的网络表清单
- PCB 布局建议
- 与这份草图一致的引脚定义头文件
