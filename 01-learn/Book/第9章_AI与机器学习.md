# 第九章：AI 与机器学习在 Arduino 上的应用

> 本章目标：了解嵌入式机器学习（TinyML）的基本概念，掌握在 Arduino 上实现线性回归、KNN 分类、异常检测和神经网络推理的方法，并完成一个 AI 智能环境监测综合项目。

---

## 9.1 嵌入式 AI 概述

### 9.1.1 什么是 TinyML？

**TinyML（Tiny Machine Learning）** 是指在资源极度受限的微控制器上运行机器学习模型的技术。

```
传统 ML 部署路径：
传感器 → 微控制器 → 云端服务器（ML推理）→ 返回结果

TinyML 部署路径：
传感器 → 微控制器（直接ML推理）→ 立即响应
```

**TinyML 的优势：**

| 特性 | 云端 ML | TinyML |
|------|---------|--------|
| 延迟 | 100ms~秒级 | 微秒~毫秒级 |
| 隐私 | 数据上传云端 | 数据不离开设备 |
| 功耗 | 需要 WiFi/4G | 极低功耗 |
| 成本 | 云服务费用 | 一次性硬件成本 |
| 离线能力 | 需要网络 | 完全离线 |

### 9.1.2 Arduino 平台的 ML 能力

| 开发板 | 处理器 | SRAM | Flash | ML 适用算法 |
|--------|--------|------|-------|------------|
| Arduino UNO | ATmega328P 8位 16MHz | 2KB | 32KB | 线性回归、KNN、简单神经网络 |
| Arduino Nano | ATmega328P 8位 16MHz | 2KB | 32KB | 同上 |
| Arduino Mega | ATmega2560 8位 16MHz | 8KB | 256KB | KNN（更大数据集）、决策树 |
| Arduino Nano 33 BLE Sense | nRF52840 32位 64MHz | 256KB | 1MB | TFLite Micro、CNN、RNN |
| ESP32 | Xtensa LX6 32位 240MHz | 520KB | 4MB | TFLite、较大神经网络 |

### 9.1.3 ML 算法选择指南

```
问题类型
  ├── 回归（预测数值）
  │     └── 线性回归 → 适合 Arduino UNO
  ├── 分类（识别类别）
  │     ├── KNN → 适合 Arduino UNO（小数据集）
  │     ├── 决策树 → 适合 Arduino Mega
  │     └── 神经网络 → 适合 Nano 33 BLE Sense
  └── 异常检测
        ├── Z-Score → 适合所有 Arduino
        └── 自编码器 → 适合 Nano 33 BLE Sense
```

---

## 9.2 实验一：在线线性回归预测

### 9.2.1 算法原理

线性回归寻找数据的线性关系 `y = w·x + b`，其中 `w` 是斜率，`b` 是截距。

**增量最小二乘法**（无需存储历史数据）：

```
每来一个新样本 (x, y)，更新以下统计量：
  n   += 1
  Sx  += x
  Sy  += y
  Sxx += x²
  Sxy += x·y

然后计算：
  w = (n·Sxy - Sx·Sy) / (n·Sxx - Sx²)
  b = (Sy - w·Sx) / n
```

### 9.2.2 硬件连接

```
DHT11 DATA → D2
DHT11 VCC  → 5V
DHT11 GND  → GND
```

### 9.2.3 核心代码

```cpp
struct LinearRegression {
  double n, Sx, Sy, Sxx, Sxy, w, b;
  
  LinearRegression() : n(0), Sx(0), Sy(0), Sxx(0), Sxy(0), w(0), b(0) {}

  void addSample(double x, double y) {
    n++; Sx += x; Sy += y; Sxx += x*x; Sxy += x*y;
    double denom = n*Sxx - Sx*Sx;
    if (abs(denom) > 1e-9) {
      w = (n*Sxy - Sx*Sy) / denom;
      b = (Sy - w*Sx) / n;
    }
  }

  double predict(double x) { return w * x + b; }
};

// 使用示例
LinearRegression model;
double timeMins = millis() / 60000.0;
model.addSample(timeMins, temperature);

// 预测未来10分钟温度
double pred = model.predict(timeMins + 10.0);
```

### 9.2.4 串口输出示例

```
[样本 #6] 时间: 0.83 min | 温度: 24.5°C | 湿度: 58.0%
  📈 温度模型: y = 0.1234 * x + 23.80
  🔮 温度预测: +5min=25.4°C  +10min=26.0°C
  📊 温度趋势: 上升趋势
  💧 湿度预测: +5min=57.2%  趋势: 平稳
```

> 📁 **代码文件**：`08_AI机器学习/01_线性回归预测/线性回归预测.ino`

---

## 9.3 实验二：KNN 环境状态分类器

### 9.3.1 算法原理

**K 近邻（KNN）** 是一种"懒惰学习"算法：

```
训练阶段：仅存储训练样本（无需计算）

预测阶段：
  1. 计算新样本与所有训练样本的欧氏距离
  2. 找到距离最近的 K 个样本
  3. 统计 K 个样本中各类别的票数
  4. 返回票数最多的类别
```

**特征归一化**（消除量纲影响）：

```cpp
// 将特征归一化到 [0, 1]
float normalize(float val, float minVal, float maxVal) {
  return (val - minVal) / (maxVal - minVal);
}
```

### 9.3.2 分类标签设计

| 类别 | 温度 | 湿度 | 光照 | 建议动作 |
|------|------|------|------|---------|
| 舒适 | 20~26°C | 40~60% | 任意 | 无需调节 |
| 炎热 | >28°C | 任意 | 任意 | 开空调/风扇 |
| 潮湿 | 任意 | >70% | 任意 | 开除湿机 |
| 干燥 | 任意 | <30% | 任意 | 开加湿器 |
| 昏暗 | 任意 | 任意 | <200 | 开灯 |

### 9.3.3 核心代码

```cpp
// 欧氏距离（归一化后）
float euclideanDistance(float* a, float* b, int n) {
  float sum = 0;
  for (int i = 0; i < n; i++) {
    float d = normalize(a[i], i) - normalize(b[i], i);
    sum += d * d;
  }
  return sqrt(sum);
}

// KNN 预测
int knnPredict(float* features, int k) {
  // 计算到所有训练样本的距离
  // 找 K 个最近邻
  // 投票决定类别
  // ...（详见完整代码）
}
```

### 9.3.4 在线学习

KNN 支持**在线学习**——通过串口命令实时添加新训练样本：

```
串口命令：add:0  → 将当前读数添加为"舒适"类别的训练样本
串口命令：add:1  → 将当前读数添加为"炎热"类别的训练样本
```

> 📁 **代码文件**：`08_AI机器学习/02_KNN分类器/KNN分类器.ino`

---

## 9.4 实验三：传感器异常检测

### 9.4.1 算法原理

**Z-Score 异常检测**：衡量数据点偏离均值的程度

```
Z = (x - μ) / σ

μ = 均值（用滑动窗口计算）
σ = 标准差（用滑动窗口计算）

判断规则：
  |Z| < 2.0  → 正常（95.4% 的正常数据落在此范围）
  |Z| < 2.5  → 可疑
  |Z| ≥ 2.5  → 异常（仅 1.2% 的正常数据会触发）
```

**突变检测**：检测相邻两次读数的差值

```
Δ = |x_t - x_{t-1}|
若 Δ > 阈值 → 突变异常
```

**EWMA 基线跟踪**（适应缓慢的环境变化）：

```
baseline_t = α · x_t + (1-α) · baseline_{t-1}

α = 0.05（平滑系数，越小越平滑）
```

### 9.4.2 防止异常污染模型

关键设计：**只用正常值更新统计模型**，防止异常值影响基线：

```cpp
Result result = detect(newValue);
if (result == NORMAL) {
  stats.push(newValue);      // 只有正常值才更新统计
  baseline.update(newValue); // 只有正常值才更新基线
}
```

### 9.4.3 测试异常检测

通过串口发送 `inject` 命令，注入模拟异常值测试系统响应：

```
串口输入：inject
串口输出：
  💉 注入模拟异常温度: 39.5°C
  🚨 异常报警: Z-Score异常
```

> 📁 **代码文件**：`08_AI机器学习/03_异常检测/异常检测.ino`

---

## 9.5 实验四：微型神经网络推理

### 9.5.1 神经网络基础

**人工神经网络（ANN）** 由多层神经元组成：

```
输入层        隐藏层1       隐藏层2       输出层
  x1 ──┐                              ┌── P(舒适)
  x2 ──┼── [8个神经元] ── [4个神经元] ──┼── P(过热)
  x3 ──┘    (ReLU)         (ReLU)     └── P(过冷)
           
每个连接都有一个权重 w，每个神经元有一个偏置 b
```

**前向传播计算**：

```
层输出 = activation(W · 输入 + b)

激活函数：
  ReLU(x)    = max(0, x)          ← 隐藏层常用
  Softmax(x) = e^xi / Σe^xj       ← 输出层（多分类）
```

### 9.5.2 在 Arduino 上部署神经网络

**工作流程：**

```
PC 端（Python）                    Arduino 端（C++）
┌──────────────────┐              ┌──────────────────┐
│ 1. 准备训练数据   │              │ 1. 读取传感器     │
│ 2. 训练神经网络   │              │ 2. 归一化特征     │
│ 3. 导出权重矩阵   │ ──部署──→   │ 3. 前向传播推理   │
│ 4. 转为C数组      │              │ 4. 输出分类结果   │
└──────────────────┘              └──────────────────┘
```

**Python 导出权重示例：**

```python
import tensorflow as tf
import numpy as np

# 训练模型（略）
model = tf.keras.Sequential([
    tf.keras.layers.Dense(8, activation='relu', input_shape=(3,)),
    tf.keras.layers.Dense(4, activation='relu'),
    tf.keras.layers.Dense(3, activation='softmax')
])
model.compile(optimizer='adam', loss='sparse_categorical_crossentropy')
model.fit(X_train, y_train, epochs=100)

# 导出权重为 C 数组
def export_weights(model):
    for i, layer in enumerate(model.layers):
        W, b = layer.get_weights()
        print(f"// Layer {i+1} weights W{i+1}[{W.shape[0]}][{W.shape[1]}]")
        print(f"const float W{i+1}[] PROGMEM = {{")
        for row in W:
            vals = ", ".join([f"{v:.6f}f" for v in row])
            print(f"  {vals},")
        print("};")

export_weights(model)
```

### 9.5.3 内存优化技巧

Arduino UNO 只有 2KB SRAM，需要将权重存储在 Flash 中：

```cpp
// 使用 PROGMEM 将权重存储在 Flash（不占 SRAM）
const float W1[] PROGMEM = { 1.234f, -0.567f, ... };

// 读取时使用 pgm_read_float()
float val = pgm_read_float(&W1[i]);
```

**内存占用对比：**

| 存储位置 | 读取方式 | 速度 | 容量 |
|---------|---------|------|------|
| SRAM | 直接访问 | 快 | 2KB（UNO） |
| Flash (PROGMEM) | pgm_read_float() | 稍慢 | 32KB（UNO） |

> 📁 **代码文件**：`08_AI机器学习/04_神经网络_前向传播/神经网络.ino`

---

## 9.6 实验五：TFLite 手势识别（进阶）

### 9.6.1 TensorFlow Lite for Microcontrollers

TFLite Micro 是专为嵌入式设备设计的推理框架：

```
TensorFlow（PC训练）
    ↓ 转换
TensorFlow Lite（移动端）
    ↓ 量化 + 优化
TFLite Micro（微控制器）
```

**模型量化**：将 FP32 权重压缩为 INT8

```
FP32：每个权重 4 字节，精度高
INT8：每个权重 1 字节，精度略低但速度快 4 倍
```

### 9.6.2 Edge Impulse 快速入门

[Edge Impulse](https://edgeimpulse.com) 是专为嵌入式 ML 设计的端到端平台：

```
数据采集 → 特征工程 → 模型训练 → 量化 → 部署到 Arduino
   ↑                                           ↓
   └──────────── 迭代优化 ────────────────────┘
```

**使用步骤：**

1. 注册 Edge Impulse 账号
2. 安装 CLI：`npm install -g edge-impulse-cli`
3. 连接 Arduino Nano 33 BLE Sense
4. 采集手势数据（每类 50+ 样本）
5. 在平台上训练模型
6. 导出 Arduino 库并安装

### 9.6.3 硬件要求

> ⚠️ 本实验需要 **Arduino Nano 33 BLE Sense**，不支持 Arduino UNO。

| 特性 | Arduino UNO | Nano 33 BLE Sense |
|------|-------------|-------------------|
| 处理器 | ATmega328P 8位 | nRF52840 32位 Cortex-M4F |
| 主频 | 16 MHz | 64 MHz |
| SRAM | 2 KB | 256 KB |
| Flash | 32 KB | 1 MB |
| IMU | ❌ | ✅ LSM9DS1（9轴） |
| TFLite | ❌ | ✅ |

> 📁 **代码文件**：`08_AI机器学习/05_TFLite_手势识别/TFLite手势识别.ino`

---

## 9.7 综合项目：AI 智能环境监测系统

### 9.7.1 系统架构

```
传感器层                  AI 处理层                  输出层
┌──────────┐            ┌──────────────────┐       ┌──────────┐
│ DHT11    │──温湿度──→ │ ① KNN 分类器     │──→   │ OLED显示 │
│ 光敏电阻 │──光照────→ │ ② 线性回归预测   │──→   │ 串口CSV  │
└──────────┘            │ ③ Z-Score异常检测│──→   │ LED指示  │
                        └──────────────────┘       │ 蜂鸣报警 │
                                                   └──────────┘
```

### 9.7.2 系统状态机

```
上电初始化
    ↓
预热阶段（10个样本）
    ↓
正常运行循环
    ├── 每5秒
    │     ├── 读取传感器（DHT11 + 光敏）
    │     ├── KNN 分类 → 更新环境状态
    │     ├── Z-Score 异常检测 → 触发报警
    │     └── 更新 OLED 显示
    └── 每10秒
          ├── 线性回归更新
          ├── 趋势预测（+10分钟）
          └── 输出 CSV 数据
```

### 9.7.3 OLED 显示页面

系统每 4 秒自动切换显示页面：

```
页面1：传感器数据        页面2：AI 分类结果
┌────────────────┐      ┌────────────────┐
│=== Sensor ===  │      │=== AI Status===│
│Temp: 24.5 C    │      │                │
│Humi: 58.0 %    │      │   Comfort      │
│LDR:  650       │      │Conf: 100%      │
│Samples: 15     │      │Normal          │
└────────────────┘      └────────────────┘

页面3：趋势预测          页面4：统计信息
┌────────────────┐      ┌────────────────┐
│=== Predict === │      │=== Stats ===   │
│Now:  24.5 C    │      │T mean: 24.2C   │
│+10m: 25.1 C    │      │T std:  0.35C   │
│Trend: Rising   │      │H mean: 57.8%   │
│Slope: 0.06/min │      │Uptime: 5min    │
└────────────────┘      └────────────────┘
```

### 9.7.4 串口 CSV 数据分析

系统输出标准 CSV 格式，可用 Python 实时分析：

```python
import serial
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

# 连接串口
ser = serial.Serial('COM3', 9600)
data = []

def update(frame):
    line = ser.readline().decode().strip()
    if ',' in line and not line.startswith('time'):
        parts = line.split(',')
        if len(parts) == 8:
            data.append({
                'time': int(parts[0])/1000,
                'temp': float(parts[1]),
                'humi': float(parts[2]),
                'status': parts[4],
                'anomaly': int(parts[7])
            })
    # 绘图（略）

ani = FuncAnimation(plt.gcf(), update, interval=1000)
plt.show()
```

> 📁 **代码文件**：`07_综合项目/06_AI智能环境监测/AI智能环境监测.ino`

---

## 9.8 ML 算法对比与选择

### 9.8.1 算法性能对比（Arduino UNO）

| 算法 | SRAM 占用 | Flash 占用 | 推理时间 | 训练方式 | 适用场景 |
|------|-----------|-----------|---------|---------|---------|
| 线性回归 | ~50B | ~500B | <1ms | 在线 | 趋势预测 |
| KNN (25样本,3特征) | ~300B | ~1KB | ~2ms | 在线/离线 | 多类分类 |
| Z-Score 异常检测 | ~200B | ~400B | <0.1ms | 在线 | 异常检测 |
| 神经网络 (3-8-4-3) | ~200B | ~400B | ~1ms | 离线 | 复杂分类 |
| TFLite Micro | 需要 Nano 33 | 需要 Nano 33 | 10~100ms | 离线 | 复杂任务 |

### 9.8.2 选择建议

```
你的问题是...
  ├── 预测一个数值（如温度）？
  │     → 线性回归（简单）或 LSTM（复杂时序）
  ├── 将数据分成几类？
  │     ├── 类别少（<10），样本少（<100）→ KNN
  │     ├── 类别多，需要高精度 → 神经网络
  │     └── 图像/音频/手势 → TFLite Micro（需要 Nano 33）
  └── 检测异常？
        ├── 单变量 → Z-Score（简单高效）
        └── 多变量 → 自编码器（需要 Nano 33）
```

---

## 9.9 本章小结

✅ 了解了 TinyML 的概念和 Arduino 平台的 ML 能力  
✅ 实现了在线线性回归，对温度趋势进行预测  
✅ 实现了 KNN 分类器，识别 5 种环境状态  
✅ 实现了 Z-Score + EWMA 异常检测系统  
✅ 实现了微型神经网络前向传播推理  
✅ 了解了 TFLite Micro 和 Edge Impulse 工作流程  
✅ 完成了 AI 智能环境监测综合项目  

### 本章涉及的代码文件

| 实验 | 代码文件 |
|------|----------|
| 线性回归预测 | `08_AI机器学习/01_线性回归预测/线性回归预测.ino` |
| KNN 分类器 | `08_AI机器学习/02_KNN分类器/KNN分类器.ino` |
| 异常检测 | `08_AI机器学习/03_异常检测/异常检测.ino` |
| 神经网络推理 | `08_AI机器学习/04_神经网络_前向传播/神经网络.ino` |
| TFLite 手势识别 | `08_AI机器学习/05_TFLite_手势识别/TFLite手势识别.ino` |
| AI 综合项目 | `07_综合项目/06_AI智能环境监测/AI智能环境监测.ino` |

### 拓展挑战

1. 🌐 为 AI 环境监测系统添加 WiFi，将分类结果推送到手机
2. 📊 用 Python + matplotlib 实时可视化串口 CSV 数据
3. 🎮 用 Edge Impulse 训练自己的手势识别模型
4. 🔧 尝试用决策树替换 KNN 分类器，比较准确率
5. 🧠 在 Arduino Nano 33 BLE Sense 上运行 TFLite 异常检测模型

### 推荐学习资源

| 资源 | 链接 |
|------|------|
| TFLite Micro 官方文档 | https://www.tensorflow.org/lite/microcontrollers |
| Edge Impulse 文档 | https://docs.edgeimpulse.com |
| 《TinyML》书籍 | Pete Warden & Daniel Situnayake 著 |
| Arduino ML 教程 | https://github.com/arduino/ArduinoTensorFlowLiteTutorials |
| Eloquent Arduino ML | https://eloquentarduino.com |

---

[← 上一章：综合项目实战](./第8章_综合项目实战.md) | [附录 →](./附录.md)