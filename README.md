# Arduino 项目与学习资料库 (Arduino Project)

本项目是一个包含 Arduino 基础教程、AI 进阶应用以及各类扩展板硬件资料与示例代码的综合性学习仓库，旨在为开发者提供从基础到高阶的完整学习路径。

## 目录结构

### 1. Arduino 基础学习教程 (`uno-learn/`)
包含 Arduino 的基础知识和各类常用模块的实践教程，并附有丰富的代码示例：
- **01_基础入门**：LED 控制、按键交互、PWM 调光、模拟量读取与串口通信等。
- **02_传感器模块**：超声波、DHT11温湿度、光敏电阻、PIR人体感应、红外遥控等各类传感器。
- **03_显示模块**：LCD1602、OLED (SSD1306)、七段数码管及 MAX7219 点阵屏等。
- **04_执行器模块**：舵机、L298N 直流电机、步进电机、继电器及蜂鸣器。
- **05_通信模块**：蓝牙 (HC-05) 和 WiFi (ESP8266) 等。
- **06_存储模块**：EEPROM、SD 卡与 RTC 时钟。
- **07_综合项目**：智能台灯、避障小车、智能浇花系统等实战项目。
- **08_AI机器学习**：介绍基础的回归预测、分类器及简单的神经网络前向传播。
- **Book**：配套的详细文本教程和书籍草稿。

### 2. Arduino 进阶与 AI 应用 (`uno-q-learn/`)
结合最新人工智能与物联网技术的进阶实战：
- **AI 部署**：基于 TensorFlow Lite 的机器视觉和语音唤醒，以及 Edge Impulse 平台的动作与异常检测。
- **大模型应用**：集成 ChatGPT API 和 Gemini Vision 等前沿 LLM 能力。
- **计算机视觉**：颜色追踪与光流运动检测。
- **Matter 协议**：Matter 智能家居设备的接入与开发。
- **MicroPython**：基于 MicroPython 的高效开发，涵盖 WiFi 交互、传感器分析和 BLE 外设。
- **Bridge RPC**：实现设备间的高效远程过程调用和仪表盘数据可视化。

### 3. Uno Q 硬件资料 (`uno-q-shield/`)
提供 Uno Q 相关的接口说明、引脚定义及硬件资料：
- JMEDIA 接口连接说明
- JMISC 接口及摄像头模块连接指南

### 4. 常用扩展板及项目 (`uno-shield/`)
收录了多种典型 Arduino 扩展板（Shield）的说明及实战代码：
- LCD1602 / LCD12864 扩展板
- 电机驱动扩展板 (Motor Shield)
- 音乐与音频扩展板 (Music Audio Shield)
- 传感器集线器 (Sensor Hub)
- 智能家居控制板 (Smart Home)
- 交通灯模拟板 (Traffic Light)
- 气象站监控 (Weather Station)

## 如何使用

1. 克隆本项目到本地：
   ```bash
   git clone https://gitee.com/zhanghaogit/arduino-project.git
   ```
2. 进入感兴趣的子目录（如 `uno-learn/01_基础入门/01_LED闪烁`）。
3. 阅读该目录下的 `README.md` 获取接线说明和实验原理。
4. 使用 [Arduino IDE](https://www.arduino.cc/en/software) 打开对应的 `.ino` 文件，选择正确的开发板和端口，编译并上传代码到你的 Arduino。

## 贡献指南

欢迎任何形式的贡献！如果你有更好的代码示例、修复了 Bug 或者发现了文档中的错误，请随时提交 Issue 或 Pull Request。

## 许可证

本项目基于开源精神分享，具体代码和文档的授权请参考各目录下的详细声明（如未特殊声明，默认为 MIT 许可证）。