# 神经网络前向传播

> 在 Arduino 上实现一个微型神经网络，完成传感器数据的智能分类。

## 📖 学习目标

- 理解人工神经网络的基本结构
- 掌握前向传播（Forward Propagation）的计算过程
- 学会将 Python/TensorFlow 训练好的模型权重部署到 Arduino

## 🧠 神经网络原理

### 网络结构

```
输入层(3)  →  隐藏层(8)  →  隐藏层(4)  →  输出层(3)

输入特征：温度、湿度、光照
输出类别：舒适(0)、过热(1)、过冷(2)
```

### 前向传播计算

```
隐藏层输出 = activation(W * x + b)
输出层输出 = softmax(W * h + b)
```

### 激活函数

| 函数 | 公式 | 特点 |
|------|------|------|
| ReLU | max(0, x) | 计算简单，常用于隐藏层 |
| Sigmoid | 1/(1+e^-x) | 输出0~1，用于二分类 |
| Tanh | (e^x-e^-x)/(e^x+e^-x) | 输出-1~1 |
| Softmax | e^xi / Σe^xj | 多分类输出概率 |

### 在 Arduino 上部署神经网络

**工作流程：**
1. 在 PC 上用 Python/TensorFlow 训练模型
2. 导出权重矩阵（W1, b1, W2, b2...）
3. 将权重以 `const float` 数组形式写入 Arduino 代码
4. Arduino 只做**推理（Inference）**，不做训练

## 🔧 硬件需求

| 组件 | 数量 | 说明 |
|------|------|------|
| Arduino UNO/Nano | 1 | 主控板 |
| DHT11 温湿度传感器 | 1 | 输入特征 |
| 光敏电阻 + 10kΩ | 1 | 输入特征 |
| RGB LED 或 3色LED | 1 | 分类结果显示 |

## 📌 硬件连接

```
DHT11 DATA   → D2
光敏电阻分压 → A0
红色 LED     → D9（舒适）
绿色 LED     → D10（过热）
蓝色 LED     → D11（过冷）
```

## 📁 文件说明

| 文件 | 说明 |
|------|------|
| `神经网络.ino` | 3层神经网络推理，含预训练权重 |

## 💡 如何更新权重

在 Python 中训练后，导出权重：

```python
import numpy as np

# 假设已训练好模型 model
W1 = model.layers[0].get_weights()[0]  # shape: (3, 8)
b1 = model.layers[0].get_weights()[1]  # shape: (8,)
# ... 以此类推

# 生成 Arduino 代码
def to_arduino_array(arr, name):
    flat = arr.flatten()
    vals = ", ".join([f"{v:.6f}f" for v in flat])
    print(f"const float {name}[] = {{{vals}}};")

to_arduino_array(W1, "W1")
to_arduino_array(b1, "b1")
```

## 📚 扩展阅读

- TensorFlow Lite for Microcontrollers
- Edge Impulse 平台
- ONNX Runtime for Embedded