# Arduino UNO Q 学习例程

> **Arduino UNO Q (Qualcomm)** 双脑架构学习例程集合

## 硬件架构：双脑设计

Arduino UNO Q 搭载 **两颗处理器**，协同工作：

| 处理器 | 型号 | 职责 | 编程语言 |
|--------|------|------|----------|
| **MCU** | Qualcomm Cortex-M33 @ 400MHz | 实时控制、传感器采集、AI 推理、硬件驱动 | Arduino C++ (`.ino`) |
| **Linux MPU** | Qualcomm QCS6490 (Linux) | 网络通信、云端 API、数据分析、Web 服务 | Python (`main.py`) |

两颗处理器通过 **Arduino Bridge** 通信：
- MCU 调用 `Bridge.call("函数名", 参数...)` → 触发 MPU 端 Python 函数
- MPU 调用 `Bridge.provide("函数名", 回调)` → 注册供 MCU 调用的函数
- MCU 使用 `Monitor.println()` 代替 `Serial.println()` 输出日志

## 每个例程的文件结构

```
例程目录/
├── 例程名.ino    ← MCU 端：Arduino C++，实时控制 + AI 推理
└── main.py       ← MPU 端：Python，云端 API + 数据分析 + Web 服务
```

> **注意**：VS Code 中 `.ino` 文件的 `#include` 报错属正常现象（未配置 Arduino includePath），
> 在 **Arduino IDE 2.x** 中编译完全正常。

---

## 例程目录

### 01 TensorFlow Lite 推理

| 例程 | MCU 职责 | MPU 职责 |
|------|----------|----------|
| [01_person_detection](01_ai_tflite/01_person_detection/) | MobileNet V1 人体检测，~200ms/帧 | 接收检测结果，统计分析，REST API |
| [02_keyword_spotting](01_ai_tflite/02_keyword_spotting/) | DS-CNN 语音关键词识别，~10ms | 接收关键词事件，执行系统动作 |
| [03_image_classification](01_ai_tflite/03_image_classification/) | MobileNetV2 图像分类，1000 类 | 接收分类结果，Top-20 统计 |

### 02 Edge Impulse

| 例程 | MCU 职责 | MPU 职责 |
|------|----------|----------|
| [01_gesture_recognition](02_ai_edge_impulse/01_gesture_recognition/) | IMU 采集 + EI 手势推理，~20ms | 接收手势事件，执行自动化动作 |
| [02_anomaly_detection](02_ai_edge_impulse/02_anomaly_detection/) | 振动数据 + EI K-means 异常评分 | 接收异常事件，告警推送，趋势分析 |

### 03 大语言模型 (LLM)

| 例程 | MCU 职责 | MPU 职责 |
|------|----------|----------|
| [01_chatgpt_api](03_ai_llm/01_chatgpt_api/) | 按键触发，OLED 显示问答 | 调用 OpenAI API，管理对话历史 |
| [02_gemini_vision_api](03_ai_llm/02_gemini_vision_api/) | 摄像头拍照，JPEG+Base64 编码 | 调用 Gemini Vision API，返回图像描述 |
| [03_sensor_ai_analysis](03_ai_llm/03_sensor_ai_analysis/) | BME280+IMU 传感器采集，定期上报 | 积累数据后调用 LLM 生成分析报告 |

### 04 计算机视觉

| 例程 | MCU 职责 | MPU 职责 |
|------|----------|----------|
| [01_color_object_tracking](04_vision/01_color_object_tracking/) | HSV 颜色检测 + PID 云台控制 | 接收追踪数据，记录轨迹，Web 仪表盘 |
| [02_optical_flow_motion](04_vision/02_optical_flow_motion/) | Lucas-Kanade 光流，运动方向/速度估计 | 接收运动事件，告警推送，速度时序图 |

### 05 Matter 智能家居

| 例程 | MCU 职责 | MPU 职责 |
|------|----------|----------|
| [01_matter_smart_home](05_matter/01_matter_smart_home/) | Matter 协议栈，设备配对，硬件控制 | 自动化规则，Web 控制面板，REST API |

### 06 Qualcomm 特色功能

| 例程 | MCU 职责 | MPU 职责 |
|------|----------|----------|
| [01_wifi6_benchmark](06_qualcomm_unique/01_wifi6_benchmark/) | WiFi 6 连接，吞吐量/延迟测试 | 接收测试结果，统计分析，Web 报告 |

### 07 MicroPython（仅 MPU）

> `07_micropython/` 目录下的例程直接在 Linux MPU 上运行 MicroPython，无需 MCU 端代码。

### 08 Bridge RPC（完整双脑示例）

> `08_bridge_rpc/` 目录下的例程包含完整的 MCU + MPU + Web UI 三层架构，
> 每个例程包含 `sketch/sketch.ino`、`python/main.py`、`assets/index.html`。

---

## 快速开始

### 1. MCU 端（Arduino IDE）

1. 安装 **Arduino IDE 2.x**
2. 添加 Arduino UNO Q 开发板支持包
3. 安装所需库（见各例程注释中的"依赖库"）
4. 打开 `.ino` 文件，选择开发板，上传

### 2. MPU 端（Linux）

```bash
# 在 Arduino UNO Q 的 Linux MPU 上运行
cd /path/to/example
python3 main.py
```

### 3. Bridge 通信

MCU 端调用 MPU 函数：
```cpp
// MCU (Arduino C++)
Bridge.call("函数名", 参数1, 参数2, ...);
```

MPU 端注册函数：
```python
# MPU (Python)
def my_function(param1, param2):
    print(f"收到: {param1}, {param2}")

Bridge.provide("函数名", my_function)
```

---

## 环境变量配置

LLM 相关例程需要设置 API Key：

```bash
# OpenAI (ChatGPT)
export OPENAI_API_KEY="sk-..."
export OPENAI_MODEL="gpt-4o-mini"  # 可选

# Google Gemini
export GEMINI_API_KEY="AIza..."
```

---

## 项目结构

```
arduino-uno-q-learn/
├── 01_ai_tflite/           # TensorFlow Lite 推理
│   ├── 01_person_detection/
│   │   ├── 01_person_detection.ino  # MCU: 人体检测推理
│   │   └── main.py                  # MPU: 结果统计+API
│   ├── 02_keyword_spotting/
│   │   ├── 02_keyword_spotting.ino  # MCU: 语音关键词识别
│   │   └── main.py                  # MPU: 动作执行+日志
│   └── 03_image_classification/
│       ├── 03_image_classification.ino  # MCU: 图像分类
│       └── main.py                       # MPU: 统计+API
├── 02_ai_edge_impulse/     # Edge Impulse
│   ├── 01_gesture_recognition/
│   │   ├── 01_gesture_recognition.ino   # MCU: 手势推理
│   │   └── main.py                       # MPU: 动作执行
│   └── 02_anomaly_detection/
│       ├── 02_anomaly_detection.ino     # MCU: 异常评分
│       └── main.py                       # MPU: 告警分析
├── 03_ai_llm/              # 大语言模型
│   ├── 01_chatgpt_api/
│   │   ├── 01_chatgpt_api.ino           # MCU: 按键+OLED
│   │   └── main.py                       # MPU: OpenAI API
│   ├── 02_gemini_vision_api/
│   │   ├── 02_gemini_vision_api.ino     # MCU: 摄像头+编码
│   │   └── main.py                       # MPU: Gemini API
│   └── 03_sensor_ai_analysis/
│       ├── 03_sensor_ai_analysis.ino    # MCU: 传感器采集
│       └── main.py                       # MPU: LLM 分析
├── 04_vision/              # 计算机视觉
│   ├── 01_color_object_tracking/
│   │   ├── 01_color_object_tracking.ino # MCU: 颜色检测+PID
│   │   └── main.py                       # MPU: 轨迹+API
│   └── 02_optical_flow_motion/
│       ├── 02_optical_flow_motion.ino   # MCU: Lucas-Kanade 光流
│       └── main.py                       # MPU: 运动事件+告警
├── 05_matter/              # Matter 智能家居
│   └── 01_matter_smart_home/
│       ├── 01_matter_smart_home.ino     # MCU: Matter 协议
│       └── main.py                       # MPU: 自动化+API
├── 06_qualcomm_unique/     # Qualcomm 特色功能
│   └── 01_wifi6_benchmark/
│       ├── 01_wifi6_benchmark.ino       # MCU: WiFi 6 测试
│       └── main.py                       # MPU: 统计+报告
├── 07_micropython/         # MicroPython (仅 MPU)
└── 08_bridge_rpc/          # 完整双脑 RPC 示例
```

---

## 参考资料

- [Arduino UNO Q 官方文档](https://docs.arduino.cc/hardware/uno-q/)
- [Qualcomm QCS6490 数据手册](https://www.qualcomm.com/products/internet-of-things/industrial/qcs6490)
- [Arduino Bridge 库文档](https://docs.arduino.cc/libraries/arduino-routerbridge/)
- [TensorFlow Lite Micro](https://www.tensorflow.org/lite/microcontrollers)
- [Edge Impulse 文档](https://docs.edgeimpulse.com/)
- [Matter 协议规范](https://csa-iot.org/developer-resource/specifications-download-request/)