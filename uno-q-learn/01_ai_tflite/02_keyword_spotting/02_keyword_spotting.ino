/**
 * TFLite 语音关键词识别例程
 * Arduino UNO Q (Qualcomm)
 * 
 * 双脑架构：
 *   MCU (Cortex-M33) → PDM 麦克风采集 + DS-CNN 推理 (~10ms)
 *   Linux MPU        → 接收关键词事件，执行系统动作，历史日志
 * 
 * 支持关键词：
 *   "yes" / "no" / "up" / "down" / "left" / "right"
 *   "on" / "off" / "stop" / "go"
 * 
 * 技术栈：
 *   - MFCC 特征提取（梅尔频率倒谱系数）
 *   - DS-CNN 模型（Depthwise Separable CNN）
 *   - INT8 量化，模型大小 ~20KB
 *   - 推理时间 ~10ms @ 400MHz
 * 
 * 硬件：
 *   MEMS 麦克风 (PDM/I2S) -> I2S 引脚
 *   或模拟麦克风 -> A0
 * 
 * 依赖库：Arduino_TensorFlowLite, Arduino_PDM (内置), Arduino_RouterBridge
 */

#include <Arduino_RouterBridge.h>  // Bridge MCU<->Linux MPU
#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <PDM.h>  // Qualcomm Arduino 内置 PDM 麦克风支持

// 关键词模型（DS-CNN INT8 量化）
#include "keyword_model_data.h"
#include "mfcc_features.h"  // MFCC 特征提取工具

// ===== 音频配置 =====
constexpr int kSampleRate      = 16000;  // 16kHz 采样率
constexpr int kAudioWindowMs   = 1000;   // 1秒滑动窗口
constexpr int kAudioSamples    = kSampleRate * kAudioWindowMs / 1000;
constexpr int kAudioStrideMs   = 20;     // 20ms 步进
constexpr int kFeatureSize     = 40;     // MFCC 特征维度
constexpr int kFeatureCount    = 49;     // 时间帧数

// ===== 关键词标签 =====
const char* kCategoryLabels[] = {
  "silence", "unknown",
  "yes", "no", "up", "down",
  "left", "right", "on", "off",
  "stop", "go"
};
constexpr int kCategoryCount = sizeof(kCategoryLabels) / sizeof(kCategoryLabels[0]);

// ===== TFLite 配置 =====
constexpr int kTensorArenaSize = 70 * 1024;  // 70KB
static uint8_t tensor_arena[kTensorArenaSize];

const tflite::Model*      model       = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor*             input       = nullptr;
TfLiteTensor*             output      = nullptr;

// ===== 音频缓冲区 =====
static int16_t audioBuffer[kAudioSamples];
static int16_t newAudioData[kSampleRate * kAudioStrideMs / 1000];
volatile bool  newDataReady = false;

// ===== PDM 回调 =====
void onPDMData() {
  int bytesAvailable = PDM.available();
  PDM.read(newAudioData, bytesAvailable);
  newDataReady = true;
}

// ===== 滑动窗口更新 =====
void updateAudioBuffer(const int16_t* newData, int newSamples) {
  int stride = newSamples;
  memmove(audioBuffer, audioBuffer + stride,
          (kAudioSamples - stride) * sizeof(int16_t));
  memcpy(audioBuffer + kAudioSamples - stride, newData,
         stride * sizeof(int16_t));
}

// ===== MFCC 特征提取 =====
void extractMFCC(const int16_t* audio, int8_t* features) {
  MFCCConfig config = {
    .sampleRate   = kSampleRate,
    .frameLen     = 512,
    .frameStep    = kSampleRate * kAudioStrideMs / 1000,
    .numMelBins   = kFeatureSize,
    .numMFCC      = kFeatureSize,
    .lowerFreq    = 20.0f,
    .upperFreq    = 4000.0f,
  };
  for (int frame = 0; frame < kFeatureCount; frame++) {
    const int16_t* frameData = audio + frame * config.frameStep;
    int8_t* frameFeatures = features + frame * kFeatureSize;
    computeMFCCFrame(frameData, config, frameFeatures);
  }
}

// ===== 推理结果 =====
struct KWSResult {
  int   topCategory;
  float topScore;
  float scores[kCategoryCount];
  unsigned long inferenceMs;
};

KWSResult runKWSInference() {
  KWSResult result = {0};
  int8_t* inputData = input->data.int8;
  extractMFCC(audioBuffer, inputData);

  unsigned long t = millis();
  interpreter->Invoke();
  result.inferenceMs = millis() - t;

  float scale     = output->params.scale;
  int   zeroPoint = output->params.zero_point;
  result.topScore    = -1;
  result.topCategory = 0;

  for (int i = 0; i < kCategoryCount; i++) {
    result.scores[i] = (output->data.int8[i] - zeroPoint) * scale;
    if (result.scores[i] > result.topScore) {
      result.topScore    = result.scores[i];
      result.topCategory = i;
    }
  }
  return result;
}

// ===== 检测阈值与防抖 =====
constexpr float kDetectionThreshold = 0.8f;
constexpr int   kSuppressMs         = 1500;

unsigned long lastDetectionTime = 0;
int           lastDetectedWord  = -1;

void handleDetection(const KWSResult& result) {
  if (result.topCategory <= 1) return;  // silence/unknown
  if (result.topScore < kDetectionThreshold) return;
  if (millis() - lastDetectionTime < kSuppressMs) return;

  lastDetectionTime = millis();
  lastDetectedWord  = result.topCategory;

  Monitor.printf("\n[KWS] 关键词: \"%s\" (置信度: %.1f%%)\n",
                kCategoryLabels[result.topCategory],
                result.topScore * 100);

  // MCU 端硬件动作
  switch (result.topCategory) {
    case 2:  // "yes"
      Monitor.println("   -> 动作: 确认");
      digitalWrite(LED_BUILTIN, HIGH);
      break;
    case 3:  // "no"
      Monitor.println("   -> 动作: 取消");
      digitalWrite(LED_BUILTIN, LOW);
      break;
    case 10: // "stop"
      Monitor.println("   -> 动作: 停止所有");
      break;
    case 11: // "go"
      Monitor.println("   -> 动作: 开始");
      break;
  }

  // ===== 通知 Linux MPU 执行更复杂的动作 =====
  // MPU 端 main.py 的 on_keyword() 函数接收此事件
  Bridge.call("on_keyword",
              kCategoryLabels[result.topCategory],
              result.topScore);
}

void setup() {
  // 初始化 Bridge（MCU <-> Linux MPU 通信）
  Bridge.begin();
  Monitor.begin();

  Monitor.println("\n========================================");
  Monitor.println("  Arduino UNO Q - TFLite 语音关键词识别");
  Monitor.println("  MCU: DS-CNN 推理 | MPU: 动作执行+日志");
  Monitor.println("========================================\n");

  pinMode(LED_BUILTIN, OUTPUT);

  // 初始化 PDM 麦克风
  PDM.onReceive(onPDMData);
  if (!PDM.begin(1, kSampleRate)) {
    Monitor.println("[ERROR] PDM 麦克风初始化失败!");
    while (1);
  }
  Monitor.println("[OK] PDM 麦克风初始化成功 (16kHz)");

  // 初始化 TFLite
  model = tflite::GetModel(g_keyword_model_data);
  static tflite::AllOpsResolver resolver;
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;
  interpreter->AllocateTensors();
  input  = interpreter->input(0);
  output = interpreter->output(0);

  Monitor.printf("[OK] 关键词模型加载成功\n");
  Monitor.printf("   模型大小: %zu bytes\n", g_keyword_model_data_len);
  Monitor.printf("   Arena 使用: %zu / %d bytes\n",
                interpreter->arena_used_bytes(), kTensorArenaSize);
  Monitor.printf("   支持关键词: ");
  for (int i = 2; i < kCategoryCount; i++) {
    Monitor.printf("\"%s\" ", kCategoryLabels[i]);
  }
  Monitor.println("\n\n请说出关键词...");
}

void loop() {
  if (!newDataReady) return;
  newDataReady = false;

  int newSamples = sizeof(newAudioData) / sizeof(int16_t);
  updateAudioBuffer(newAudioData, newSamples);

  static unsigned long lastInference = 0;
  if (millis() - lastInference < kAudioStrideMs) return;
  lastInference = millis();

  KWSResult result = runKWSInference();
  handleDetection(result);

  // 实时音量指示
  static unsigned long lastViz = 0;
  if (millis() - lastViz > 200) {
    lastViz = millis();
    int32_t energy = 0;
    for (int i = 0; i < 160; i++) energy += abs(newAudioData[i]);
    energy /= 160;
    int bars = map(energy, 0, 2000, 0, 20);
    Monitor.printf("\r音量: [");
    for (int i = 0; i < 20; i++) Monitor.print(i < bars ? "#" : " ");
    Monitor.printf("] %s (%.0f%%)  ",
                  result.topCategory > 1 ? kCategoryLabels[result.topCategory] : "...",
                  result.topScore * 100);
  }
}