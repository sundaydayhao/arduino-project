/**
 * Edge Impulse 手势识别例程
 * Arduino UNO Q (Qualcomm)
 *
 * 双脑架构：
 *   MCU (Cortex-M33) → IMU 采集 + Edge Impulse 推理，~20ms
 *   Linux MPU        → 接收手势事件，执行自动化动作，历史统计
 *
 * 手势类别：circle / wave / punch / flex / idle
 * 传感器：LSM6DS3 IMU (加速度计 + 陀螺仪)
 *
 * 依赖库：Arduino_LSM6DS3, ei_arduino (Edge Impulse SDK), Arduino_RouterBridge
 */

#include <Arduino_RouterBridge.h>
#include <Arduino_LSM6DS3.h>
#include <gesture_recognition_inferencing.h>  // Edge Impulse 生成的库

// ===== Edge Impulse 配置 =====
// EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE = 采样点数 * 轴数
// EI_CLASSIFIER_LABEL_COUNT          = 手势类别数
constexpr float kSampleFreqHz  = 119.0f;  // LSM6DS3 采样率
constexpr int   kSuppressMs    = 500;     // 检测后抑制时间

// 采样缓冲区
static float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
static int   featureIdx = 0;

unsigned long lastDetectionMs = 0;

// ===== 采集 IMU 数据 =====
bool collectSample() {
  if (!IMU.accelerationAvailable() || !IMU.gyroscopeAvailable()) return false;

  float ax, ay, az, gx, gy, gz;
  IMU.readAcceleration(ax, ay, az);
  IMU.readGyroscope(gx, gy, gz);

  if (featureIdx + 6 <= EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
    features[featureIdx++] = ax;
    features[featureIdx++] = ay;
    features[featureIdx++] = az;
    features[featureIdx++] = gx;
    features[featureIdx++] = gy;
    features[featureIdx++] = gz;
  }
  return true;
}

// ===== Edge Impulse 推理 =====
struct GestureResult {
  const char*   label;
  float         confidence;
  unsigned long inferenceMs;
};

GestureResult runInference() {
  GestureResult result = {"idle", 0.0f, 0};

  signal_t signal;
  numpy::signal_from_buffer(features, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);

  ei_impulse_result_t eiResult;
  unsigned long t = millis();
  EI_IMPULSE_ERROR err = run_classifier(&signal, &eiResult, false);
  result.inferenceMs = millis() - t;

  if (err != EI_IMPULSE_OK) {
    Monitor.printf("[ERROR] 推理失败: %d\n", err);
    return result;
  }

  // 找最高置信度
  float topScore = 0;
  int   topIdx   = 0;
  for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    if (eiResult.classification[i].value > topScore) {
      topScore = eiResult.classification[i].value;
      topIdx   = i;
    }
  }
  result.label      = ei_classifier_inferencing_categories[topIdx];
  result.confidence = topScore;
  return result;
}

void setup() {
  Bridge.begin();
  Monitor.begin();

  Monitor.println("========================================");
  Monitor.println("  Arduino UNO Q - Edge Impulse 手势识别");
  Monitor.println("  MCU: EI 推理 | MPU: 动作执行+统计");
  Monitor.println("========================================");

  if (!IMU.begin()) {
    Monitor.println("[ERROR] IMU 初始化失败!");
    while (1) delay(1000);
  }
  Monitor.printf("[OK] IMU 初始化成功 (加速度: %.0fHz, 陀螺仪: %.0fHz)\n",
                IMU.accelerationSampleRate(),
                IMU.gyroscopeSampleRate());
  Monitor.println("开始手势识别...\n");
}

void loop() {
  // 持续采集 IMU 数据
  if (!collectSample()) {
    delayMicroseconds((int)(1000000.0f / kSampleFreqHz));
    return;
  }

  // 缓冲区满时运行推理
  if (featureIdx < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) return;
  featureIdx = 0;  // 重置缓冲区

  GestureResult result = runInference();

  // 过滤 idle 和低置信度
  if (strcmp(result.label, "idle") == 0) return;
  if (result.confidence < 0.75f) return;
  if (millis() - lastDetectionMs < kSuppressMs) return;

  lastDetectionMs = millis();

  Monitor.printf("[手势] %s (%.1f%%) | 推理: %lums\n",
                result.label, result.confidence * 100, result.inferenceMs);

  // ===== 通知 Linux MPU =====
  // MPU 端 main.py 的 on_gesture() 函数接收此事件
  Bridge.call("on_gesture",
              result.label,
              result.confidence,
              (int)result.inferenceMs);
}