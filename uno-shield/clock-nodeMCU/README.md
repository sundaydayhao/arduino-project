# NodeMCU 六位数码管网络时钟

这个版本把主控从 `Arduino Uno` 换成了 `NodeMCU(ESP8266)`，时间来源也从 `DS1302 RTC` 改成了 `NTP`，温湿度改成了通过网络天气 API 获取。

当前实现默认支持：

- 时间：`HH.MM.SS`
- 日期：`YY.MM.DD`
- 温度：`t23.5C`
- 湿度：`h56.2H`

代码文件见：[clock-nodeMCU.ino](D:\workspace\arduino-project\uno-shield\clock-nodeMCU\clock-nodeMCU.ino)

## 1. 方案变化

原来的 Uno 版本是直接用 `8 段线 + 6 位选` 驱动数码管，再外接 `DS1302` 和 `DHT`。  
这个接法放到 `NodeMCU` 上不成立，因为 ESP8266 可安全使用的 IO 不够。

所以这里改成了新的结构：

- `NodeMCU` 只负责联网、NTP 同步和天气 API 请求
- 显示改成 `2 片 74HCT595` 级联输出
- 保留 6 位独立数码管动态扫描
- 去掉 `DS1302`
- 去掉本地 `DHT11 / DHT22`

如果你必须保留原来“完全直驱数码管”的接法，那就不适合直接换成 `NodeMCU`。

## 2. 器件清单

- `NodeMCU(ESP8266)` 1 块
- 独立一位七段数码管 6 个
- `74HC595 / 74HCT595` 2 片
  你现在这类 `HG74HC595N` 符号属于 `74HC595` 命名体系，下文接线名按它的 `SER / SRCLK / RCLK / SRCLR# / QH'` 来写  
  推荐优先用 `74HCT595`，因为 `NodeMCU` 是 `3.3V` 逻辑，`74HC595` 在 `5V` 供电下高电平门限更紧，兼容性不如 `HCT`
- `BC327` PNP 三极管 6 个
- `330R` 电阻 8 个
  每个段线一只，`a b c d e f g dp` 各 1 个
- `1k` 电阻 6 个
  每个数码管位选三极管基极一只
- `10k` 电阻 6 个
  每个位选三极管基极上拉一只
- `0.1uF` 去耦电容若干
  每片 `74HCT595` 电源旁建议各放 1 个
- `5V` 电源 1 份
  数码管和移位寄存器建议由稳定 `5V` 供电
- 面包板、导线若干

## 3. 引脚分配

NodeMCU 只占 3 根控制线：

- `D7(GPIO13)` -> `SER`
- `D5(GPIO14)` -> `SRCLK`
- `D6(GPIO12)` -> `RCLK`

其余固定连接建议：

- `OE#` -> `GND`
- `SRCLR#` -> `VCC`

程序默认假设：

- 第一片 `74HCT595` 负责 8 根段线
- 第二片 `74HCT595` 负责 6 根位选
- 两片通过 `QH' -> SER` 级联

如果你用的是这张图里的 `HG74HC595N`，常见别名对应如下：

- `SER` = `DS`
- `SRCLK` = `SHCP` = `SRCK`
- `RCLK` = `STCP` = `LATCH`
- `SRCLR#` = `MR#`
- `QH'` = `Q7'` = `Q7S`
- `QA~QH` = `Q0~Q7`

如果你实际级联顺序不同，需要同步调整代码里 `writeShiftRegisters()` 的字节输出顺序。

## 4. 数码管驱动方式

当前仍按 **18101BS 共阳型数码管** 组织。

逻辑不变：

- 段线输出 `LOW` 表示该段点亮
- 位选输出 `LOW` 让对应 `BC327` 导通
- 程序轮流扫描 6 位

推荐连接方式：

- 第一片 `74HCT595` 的 `QA~QH` 接 `a b c d e f g dp`
- 每条段线串一个 `330R`
- 第二片 `74HCT595` 的 `QA~QF` 分别接 6 个 `BC327` 基极，前面串 `1k`
- 每个 `BC327` 基极再通过 `10k` 上拉到 `5V`
- `BC327` 发射极接 `5V`
- `BC327` 集电极接对应数码管公共阳极

注意：

- 这里推荐 `74HCT595` 工作在 `5V`
- `NodeMCU` 输出的 `3.3V` 高电平通常能可靠驱动 `74HCT595`
- 不建议直接让 `NodeMCU` 去承担 6 位数码管的段电流

## 5. 级联连接示意

下面是逻辑示意：

```text
NodeMCU D7(GPIO13) ---- SER(74HCT595 #1)
NodeMCU D5(GPIO14) ---- SRCLK
NodeMCU D6(GPIO12) ---- RCLK

74HCT595 #1 OE# ------- GND
74HCT595 #2 OE# ------- GND
74HCT595 #1 SRCLR# ---- VCC
74HCT595 #2 SRCLR# ---- VCC

74HCT595 #1 QH' ------- SER(74HCT595 #2)

74HCT595 #1 QA -------- 330R -------- 段 a
74HCT595 #1 QB -------- 330R -------- 段 b
74HCT595 #1 QC -------- 330R -------- 段 c
74HCT595 #1 QD -------- 330R -------- 段 d
74HCT595 #1 QE -------- 330R -------- 段 e
74HCT595 #1 QF -------- 330R -------- 段 f
74HCT595 #1 QG -------- 330R -------- 段 g
74HCT595 #1 QH -------- 330R -------- 段 dp

74HCT595 #2 QA ---- 1k ---- BC327(1) 基极
74HCT595 #2 QB ---- 1k ---- BC327(2) 基极
74HCT595 #2 QC ---- 1k ---- BC327(3) 基极
74HCT595 #2 QD ---- 1k ---- BC327(4) 基极
74HCT595 #2 QE ---- 1k ---- BC327(5) 基极
74HCT595 #2 QF ---- 1k ---- BC327(6) 基极

每个 BC327:
基极 --- 10k --- 5V
发射极 --------- 5V
集电极 --------- 对应数码管公共阳极
```

## 6. NTP 与天气 API

### NTP

代码默认使用：

- `ntp1.aliyun.com`
- `ntp.ntsc.ac.cn`
- `pool.ntp.org`

时区固定为：

- `UTC+8`

也就是默认按中国时区显示。

### 天气 API

默认示例使用 `Open-Meteo` 当前天气接口，配置在代码顶部：

```cpp
const char *WEATHER_URL =
    "https://api.open-meteo.com/v1/forecast"
    "?latitude=31.2304"
    "&longitude=121.4737"
    "&current=temperature_2m,relative_humidity_2m"
    "&timezone=Asia%2FShanghai";
```

这个地址默认取的是上海坐标。  
你只需要改：

- `latitude`
- `longitude`
- `timezone`

就可以切换到自己的城市。

程序当前从响应里提取：

- `temperature_2m`
- `relative_humidity_2m`

如果你要换成别的天气服务，也可以继续沿用当前框架，只需要改：

- `WEATHER_URL`
- `fetchWeatherNow()` 里的字段解析

## 7. 代码配置项

在代码最上面先改这几个配置：

```cpp
const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
```

如果不改，程序能编译，但无法联网。

## 8. 显示逻辑

程序自动轮播：

- 时间：`12.34.56`
- 日期：`26.04.06`
- 温度：`t23.5C`
- 湿度：`h56.2H`

状态页说明：

- `nt---`：WiFi 已连上，但 NTP 还没同步成功
- `no---`：当前 WiFi 未连接
- `t----`：温度 API 还没拿到有效值
- `h----`：湿度 API 还没拿到有效值

## 9. 软件依赖

基于 `ESP8266` Arduino Core，自带这些头文件：

- `ESP8266WiFi.h`
- `ESP8266HTTPClient.h`
- `WiFiClientSecureBearSSL.h`
- `time.h`

这个版本不再需要：

- `DHT sensor library`
- `Adafruit Unified Sensor`
- `DS1302` 相关库

## 10. 烧录步骤

1. 在 Arduino IDE 安装 `ESP8266` 开发板支持包
2. 开发板选择 `NodeMCU 1.0 (ESP-12E Module)`
3. 打开 [clock-nodeMCU.ino](D:\workspace\arduino-project\uno-shield\clock-nodeMCU\clock-nodeMCU.ino)
4. 填好 `WIFI_SSID` 和 `WIFI_PASSWORD`
5. 按实际城市修改 `WEATHER_URL`
6. 编译并上传

第一次启动后：

- 先连 WiFi
- 然后自动同步 NTP
- 之后每隔一段时间拉取一次天气温湿度

## 11. 当前限制

- 这个版本默认是 **6 个独立一位共阳数码管**
- 默认是 **2 片 74HCT595 + 6 个 PNP 位选**
- 没有保留 Uno 版本里的 `DS1302` 掉电保持
- 断网时温湿度显示保留上一次成功获取的值
- 如果设备从未联网成功，时间页会显示 `nt---` 或 `no---`
- 当前没有保留按键设置时间，因为时间来源已经切到 NTP

## 12. 可继续优化的方向

- 增加 `WiFiManager` 做配网，不再把 WiFi 写死在代码里
- 把天气 API 换成你自己的服务商，例如和风天气或其他带 key 的接口
- 增加亮度控制，例如光敏电阻或夜间自动降亮
- 加入本地温湿度传感器，做“本地实测值”和“网络天气值”双模式显示
- 把启动状态细分成 `WiFi 连接中 / NTP 同步中 / API 获取中`
