/**
 * Edge Impulse 异常检测例程
 * Arduino UNO Q (Qualcomm)
 *
 * 双脑架构：
 *   MCU (Cortex-M33) → 传感器采集 + EI K-means 异常评分，实时检测
 *   Linux MPU        → 接收异常事件，告警推送，趋势分析，REST API
 *
 * 传感器：振动/温度/电流（可配置）
 * 算法：Edge Impulse K-means 聚类异常检测
 *
 * 依赖库：Arduino_LSM6DS3, ei_arduino (Edge Impulse SDK), Arduino_RouterBridge
 */

#include <Arduino_RouterBridge.h>
#include <Arduino_LSM6DS3.h>
#include <anomaly_detection_inferencing.h>  // Edge Impulse 生成的库

// ===== 配置 =====
constexpr float kAnomalyThreshold = 0.6f;  // 异常评分阈值
constexpr int   kWindowMs         = 100;   // 采样窗口

static float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
static int   featureIdx = 0;

unsigned long lastSampleMs = 0;
int totalSamples  = 0;
int anomalyCount  = 0;

// ===== 采集传感器数据 =====
bool collectSample() {
  if (!IMU.accelerationAvailable()) return false;

  float ax, ay, az;
  IMU.readAcceleration(ax, ay, az);

  if (featureIdx + 3 <= EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
    features[featureIdx++] = ax;
    features[featureIdx++] = ay;
    features[featureIdx++] = az;
  }
  return true;
}

// ===== 运行 EI 推理 =====
struct AnomalyResult {
  float         score;
  bool          isAnomaly;
  unsigned long inferenceMs;
};

AnomalyResult runInference() {
  AnomalyResult result = {0.0f, false, 0};

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

#if EI_CLASSIFIER_HAS_ANOMALY == 1
  result.score     = eiResult.anomaly;
  result.isAnomaly = (result.score > kAnomalyThreshold);
#endif

  return result;
}

void setup() {
  Bridge.begin();
  Monitor.begin();

  Monitor.println("========================================");
  Monitor.println("  Arduino UNO Q - Edge Impulse 异常检测");
  Monitor.println("  MCU: EI 推理 | MPU: 告警+趋势分析");
  Monitor.println("========================================");

  if (!IMU.begin()) {
    Monitor.println("[ERROR] IMU 初始化失败!");
    while (1) delay(1000);
  }
  Monitor.println("[OK] IMU 初始化成功");
  Monitor.printf("  异常阈值: %.2f\n", kAnomalyThreshold);
  Monitor.printf("  输入帧大小: %d\n\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
  Monitor.println("开始异常检测...");
}

void loop() {
  // 按固定频率采集
  if (millis() - lastSampleMs < 8) return;  // ~125Hz
  lastSampleMs = millis();

  if (!collectSample()) return;

  // 缓冲区满时推理
  if (featureIdx < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) return;
  featureIdx = 0;

  AnomalyResult result = runInference();
  totalSamples++;
  if (result.isAnomaly) anomalyCount++;

  if (result.isAnomaly) {
    Monitor.printf("[!!!] 异常! 评分: %.4f (阈值: %.2f) | %lums\n",
                  result.score, kAnomalyThreshold, result.inferenceMs);
  } else {
    Monitor.printf("[ OK] 正常  评分: %.4f | %lums | 异常率: %.1f%%\n",
                  result.score, result.inferenceMs,
                  100.0f * anomalyCount / totalSamples);
  }

  // ===== 发送结果到 Linux MPU =====
  // MPU 端 main.py 的 update_anomaly() 函数接收此数据
  Bridge.call("update_anomaly",
              result.score,
              (int)result.isAnomaly,
              (int)result.inferenceMs);
}