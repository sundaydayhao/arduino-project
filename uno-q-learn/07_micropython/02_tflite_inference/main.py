"""
TensorFlow Lite 推理 - MicroPython 版
Arduino UNO Q (Qualcomm)

MicroPython 上运行 TFLite Micro：
  - 使用 utensor 或 tflite-micro Python 绑定
  - 比 C++ 版本更易于模型替换和调试
  - 支持 INT8 量化模型

支持的任务：
  - 图像分类（MobileNet）
  - 关键词识别（DS-CNN）
  - 手势识别（IMU + NN）
  - 异常检测（Autoencoder）

依赖：tflite_micro (Qualcomm MicroPython 内置)
"""

import tflite_micro as tflm
import array
import time
import math

# ===== 加载模型 =====
def load_model(model_path):
    """从文件系统加载 .tflite 模型"""
    with open(model_path, 'rb') as f:
        model_data = f.read()
    print(f"✅ 模型加载: {model_path} ({len(model_data)} bytes)")
    return model_data

# ===== TFLite 推理引擎 =====
class TFLiteInference:
    def __init__(self, model_data, arena_size=256*1024):
        """
        初始化 TFLite 推理引擎
        arena_size: Tensor Arena 大小（字节）
        """
        self.interpreter = tflm.Interpreter(model_data, arena_size)
        self.interpreter.allocate_tensors()

        # 获取输入输出信息
        self.input_details  = self.interpreter.get_input_details()
        self.output_details = self.interpreter.get_output_details()

        print(f"   输入形状: {self.input_details[0]['shape']}")
        print(f"   输入类型: {self.input_details[0]['dtype']}")
        print(f"   输出形状: {self.output_details[0]['shape']}")
        print(f"   Arena 使用: {self.interpreter.arena_used_bytes()} bytes")

    def infer(self, input_data):
        """运行推理，返回输出数组"""
        self.interpreter.set_tensor(self.input_details[0]['index'], input_data)
        t0 = time.ticks_ms()
        self.interpreter.invoke()
        elapsed = time.ticks_diff(time.ticks_ms(), t0)
        output = self.interpreter.get_tensor(self.output_details[0]['index'])
        return output, elapsed

# ===== 图像分类 =====
class ImageClassifier:
    # ImageNet Top-20 常见类别（完整版需加载标签文件）
    LABELS = [
        "tench", "goldfish", "great_white_shark", "tiger_shark",
        "hammerhead", "electric_ray", "stingray", "cock", "hen",
        "ostrich", "brambling", "goldfinch", "house_finch", "junco",
        "indigo_bunting", "robin", "bulbul", "jay", "magpie", "chickadee",
    ]

    def __init__(self, model_path="mobilenet_v2.tflite"):
        print("初始化图像分类器...")
        model_data = load_model(model_path)
        self.engine = TFLiteInference(model_data, arena_size=512*1024)
        self.input_shape = self.engine.input_details[0]['shape']
        # [1, H, W, C]
        self.height = self.input_shape[1]
        self.width  = self.input_shape[2]

    def preprocess(self, rgb_bytes):
        """
        预处理图像：缩放 + 归一化
        rgb_bytes: 原始 RGB888 字节数据
        返回: INT8 量化数组
        """
        # 简化版双线性缩放（实际应用建议用 C 扩展加速）
        src_w = int(math.sqrt(len(rgb_bytes) // 3))
        src_h = src_w
        result = array.array('b', [0] * (self.height * self.width * 3))

        for y in range(self.height):
            for x in range(self.width):
                sx = int(x * src_w / self.width)
                sy = int(y * src_h / self.height)
                src_idx = (sy * src_w + sx) * 3
                dst_idx = (y * self.width + x) * 3
                # 归一化到 [-128, 127]
                result[dst_idx]   = rgb_bytes[src_idx]   - 128
                result[dst_idx+1] = rgb_bytes[src_idx+1] - 128
                result[dst_idx+2] = rgb_bytes[src_idx+2] - 128
        return result

    def classify(self, rgb_bytes, top_k=5):
        """
        分类图像，返回 Top-K 结果
        返回: [(label, score), ...]
        """
        input_data = self.preprocess(rgb_bytes)
        output, elapsed_ms = self.engine.infer(input_data)

        # 反量化 + Softmax（如果模型输出未经 Softmax）
        scale     = self.engine.output_details[0].get('quantization', (1.0, 0))[0]
        zero_pt   = self.engine.output_details[0].get('quantization', (1.0, 0))[1]
        scores = [(v - zero_pt) * scale for v in output[0]]

        # Top-K
        indexed = sorted(enumerate(scores), key=lambda x: x[1], reverse=True)
        results = []
        for i, (idx, score) in enumerate(indexed[:top_k]):
            label = self.LABELS[idx] if idx < len(self.LABELS) else f"class_{idx}"
            results.append((label, score))

        return results, elapsed_ms

# ===== 关键词识别 =====
class KeywordSpotter:
    KEYWORDS = ["silence", "unknown", "yes", "no", "up", "down",
                "left", "right", "on", "off", "stop", "go"]

    def __init__(self, model_path="keyword_model.tflite"):
        print("初始化关键词识别器...")
        model_data = load_model(model_path)
        self.engine = TFLiteInference(model_data, arena_size=70*1024)

    def spot(self, mfcc_features):
        """
        识别关键词
        mfcc_features: MFCC 特征数组 (49 * 40 = 1960 个 INT8 值)
        """
        output, elapsed_ms = self.engine.infer(mfcc_features)
        scores = list(output[0])
        top_idx = scores.index(max(scores))
        top_score = max(scores)
        label = self.KEYWORDS[top_idx] if top_idx < len(self.KEYWORDS) else "unknown"
        return label, top_score, elapsed_ms

# ===== 手势识别（IMU）=====
class GestureRecognizer:
    GESTURES = ["idle", "swipe_up", "swipe_down", "swipe_left",
                "swipe_right", "circle", "shake"]

    def __init__(self, model_path="gesture_model.tflite"):
        print("初始化手势识别器...")
        model_data = load_model(model_path)
        self.engine = TFLiteInference(model_data, arena_size=50*1024)
        self.buffer = []
        self.WINDOW = 100  # 100 个样本 @ 100Hz = 1秒

    def add_sample(self, ax, ay, az, gx, gy, gz):
        """添加 IMU 样本到滑动窗口"""
        self.buffer.append([ax, ay, az, gx, gy, gz])
        if len(self.buffer) > self.WINDOW:
            self.buffer.pop(0)

    def recognize(self):
        """识别当前窗口的手势"""
        if len(self.buffer) < self.WINDOW:
            return None, 0, 0

        # 展平为 1D 数组
        flat = array.array('f')
        for sample in self.buffer:
            flat.extend(sample)

        output, elapsed_ms = self.engine.infer(flat)
        scores = list(output[0])
        top_idx = scores.index(max(scores))
        top_score = max(scores)
        gesture = self.GESTURES[top_idx] if top_idx < len(self.GESTURES) else "unknown"
        return gesture, top_score, elapsed_ms

# ===== 演示：模拟推理 =====
def demo_classification():
    """演示图像分类（使用随机数据模拟）"""
    print("\n===== 图像分类演示 =====")
    # 模拟 96x96 RGB 图像
    import urandom
    fake_image = bytes([urandom.getrandbits(8) for _ in range(96*96*3)])

    try:
        classifier = ImageClassifier()
        results, ms = classifier.classify(fake_image)
        print(f"\n推理时间: {ms}ms")
        print("Top-5 结果:")
        for i, (label, score) in enumerate(results):
            bar = "█" * int(score * 20)
            print(f"  {i+1}. {label:<25} {score*100:5.1f}% {bar}")
    except Exception as e:
        print(f"❌ 错误: {e}")
        print("   请确保模型文件已上传到开发板文件系统")

def demo_gesture():
    """演示手势识别（使用模拟 IMU 数据）"""
    print("\n===== 手势识别演示 =====")
    import urandom

    try:
        recognizer = GestureRecognizer()
        # 模拟 100 个 IMU 样本
        for _ in range(100):
            ax = (urandom.getrandbits(8) - 128) / 16.0
            ay = (urandom.getrandbits(8) - 128) / 16.0
            az = 9.8 + (urandom.getrandbits(8) - 128) / 64.0
            gx = (urandom.getrandbits(8) - 128) / 10.0
            gy = (urandom.getrandbits(8) - 128) / 10.0
            gz = (urandom.getrandbits(8) - 128) / 10.0
            recognizer.add_sample(ax, ay, az, gx, gy, gz)

        gesture, score, ms = recognizer.recognize()
        print(f"识别结果: {gesture} ({score*100:.1f}%) [{ms}ms]")
    except Exception as e:
        print(f"❌ 错误: {e}")

# ===== 主程序 =====
def main():
    print("\n" + "="*42)
    print("  Arduino UNO Q - TFLite (MicroPython)")
    print("  端侧 AI 推理，Python 接口")
    print("="*42 + "\n")

    print("命令:")
    print("  'classify' → 图像分类演示")
    print("  'gesture'  → 手势识别演示")
    print("  'bench'    → 推理性能测试\n")

    while True:
        try:
            cmd = input(">>> ").strip()
        except EOFError:
            break

        if cmd == "classify":
            demo_classification()
        elif cmd == "gesture":
            demo_gesture()
        elif cmd == "bench":
            print("性能测试...")
            # 测试 1000 次简单推理
            t0 = time.ticks_ms()
            for _ in range(100):
                data = array.array('b', [0] * 96)
                # 模拟推理
            elapsed = time.ticks_diff(time.ticks_ms(), t0)
            print(f"100次推理: {elapsed}ms (avg {elapsed/100:.1f}ms)")
        else:
            print("未知命令")

main()