# TensorFlow Lite 手势识别

> 使用 TensorFlow Lite for Microcontrollers 在 Arduino Nano 33 BLE Sense 上实现实时手势识别。

## ⚠️ 硬件要求

本实验需要 **Arduino Nano 33 BLE Sense**（或 BLE Sense Rev2），不支持 Arduino UNO。

| 特性 | Arduino UNO | Arduino Nano 33 BLE Sense |
|------|-------------|--------------------------|
| 处理器 | ATmega328P (8位) | nRF52840 (32位 Cortex-M4) |
| 主频 | 16 MHz | 64 MHz |
| Flash | 32 KB | 1 MB |
| SRAM | 2 KB | 256 KB |
| IMU | ❌ | ✅ LSM9DS1 |
| TFLite 支持 | ❌ | ✅ |

## 📖 学习目标

- 了解 TensorFlow Lite for Microcontrollers 框架
- 理解模型量化（Quantization）的概念
- 掌握 Edge Impulse 工作流程
- 学会在嵌入式设备上部署 ML 模型

## 🧠 技术原理

### TensorFlow Lite for Microcontrollers

TFLite Micro 是 TensorFlow Lite 的嵌入式版本：

```
PC 端（Python）                    Arduino 端（C++）
┌─────────────────┐               ┌─────────────────┐
│ 收集训练数据     │               │ 读取 IMU 数据    │
│ 训练 TF 模型    │               │ 特征提取         │
│ 转换为 TFLite   │  → 部署 →    │ TFLite 推理      │
│ 量化（INT8）    │               │ 输出预测结果     │
└─────────────────┘               └─────────────────┘
```

### 模型量化

将 32位浮点（FP32）权重压缩为 8位整数（INT8）：

```
FP32 模型：100KB → INT8 量化后：25KB（压缩4倍）
精度损失：通常 < 1%
推理速度：提升 2~4 倍
```

### 手势识别流程

```
IMU 采集（加速度计 + 陀螺仪）
    ↓
滑动窗口（119 个采样点 × 6 轴）
    ↓
特征提取（归一化）
    ↓
TFLite 模型推理
    ↓
手势分类（上挥/下挥/左挥/右挥）
```

## 🔧 硬件需求

| 组件 | 数量 | 说明 |
|------|------|------|
| Arduino Nano 33 BLE Sense | 1 | 必须，含 IMU |
| USB-C 数据线 | 1 | 连接电脑 |

## 📦 所需库

在 Arduino IDE 库管理器中安装：

| 库名称 | 版本 | 说明 |
|--------|------|------|
| `Arduino_TensorFlowLite` | ≥ 2.4.0 | TFLite Micro 运行时 |
| `Arduino_LSM9DS1` | ≥ 1.1.0 | IMU 传感器驱动 |

## 📁 文件说明

| 文件 | 说明 |
|------|------|
| `TFLite手势识别.ino` | 主程序：IMU 采集 + TFLite 推理 |
| `gesture_model.h` | 量化后的 TFLite 模型（C 数组格式） |

## 🚀 快速开始（使用 Edge Impulse）

Edge Impulse 是专为嵌入式 ML 设计的平台，可以大幅简化工作流程：

### 步骤 1：数据采集

```bash
# 安装 Edge Impulse CLI
npm install -g edge-impulse-cli

# 连接 Arduino Nano 33 BLE Sense
edge-impulse-data-forwarder
```

### 步骤 2：在 Edge Impulse 平台训练

1. 访问 [studio.edgeimpulse.com](https://studio.edgeimpulse.com)
2. 创建项目 → 采集数据（每个手势 50+ 样本）
3. 设计 Impulse（特征提取 + 分类器）
4. 训练模型
5. 部署 → Arduino 库

### 步骤 3：部署到 Arduino

```
Edge Impulse → 部署 → Arduino 库 → 下载 .zip
Arduino IDE → 项目 → 加载库 → 添加 .ZIP 库
```

## 💡 应用场景

- 手势控制智能家居
- 体感游戏控制器
- 康复训练监测
- 运动姿态分析

## 📚 参考资源

- [TFLite Micro 官方文档](https://www.tensorflow.org/lite/microcontrollers)
- [Edge Impulse 文档](https://docs.edgeimpulse.com)
- [Arduino ML 示例](https://github.com/arduino/ArduinoTensorFlowLiteTutorials)
- [《TinyML》书籍](https://www.oreilly.com/library/view/tinyml/9781492052036/)