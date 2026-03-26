/**
 * TFLite MobileNetV2 图像分类例程
 * Arduino UNO Q (Qualcomm)
 *
 * 双脑架构：
 *   MCU (Cortex-M33) → MobileNetV2 推理，Top-5 分类，~200ms/帧
 *   Linux MPU        → 接收分类结果，统计分析，REST API，Web 仪表盘
 *
 * 模型：MobileNetV2 224x224 RGB INT8 量化，1000 类 ImageNet
 * 性能：~200ms/帧 @ Cortex-M33 400MHz
 *
 * 依赖库：Arduino_TensorFlowLite, Arduino_RouterBridge
 */

#include <Arduino_RouterBridge.h>
#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>

#include "camera_provider.h"
#include "mobilenet_v2_model_data.h"
#include "imagenet_labels.h"  // 1000 类标签

// ===== 模型配置 =====
constexpr int kImgW    = 224;
constexpr int kImgH    = 224;
constexpr int kImgC    = 3;    // RGB
constexpr int kTopK    = 5;    // Top-5 输出
constexpr int kNumClasses = 1000;

constexpr int kTensorArenaSize = 512 * 1024;  // 512KB
static uint8_t tensor_arena[kTensorArenaSize];

const tflite::Model*      model       = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor*             input       = nullptr;
TfLiteTensor*             output      = nullptr;

// ===== Top-K 结果 =====
struct ClassResult {
  int   classId;
  float score;
};

void getTopK(const float* scores, int n, ClassResult* topK, int k) {
  for (int i = 0; i < k; i++) {
    topK[i] = {-1, -1.0f};
  }
  for (int i = 0; i < n; i++) {
    if (scores[i] > topK[k-1].score) {
      topK[k-1] = {i, scores[i]};
      // 插入排序
      for (int j = k-1; j > 0 && topK[j].score > topK[j-1].score; j--) {
        ClassResult tmp = topK[j]; topK[j] = topK[j-1]; topK[j-1] = tmp;
      }
    }
  }
}

void setup() {
  Bridge.begin();
  Monitor.begin();

  Monitor.println("========================================");
  Monitor.println("  Arduino UNO Q - TFLite 图像分类");
  Monitor.println("  MCU: MobileNetV2 推理 | MPU: 统计+API");
  Monitor.println("========================================");

  if (!initCamera()) {
    Monitor.println("[ERROR] 摄像头初始化失败!");
    while (1) delay(1000);
  }
  Monitor.println("[OK] 摄像头初始化成功");

  model = tflite::GetModel(g_mobilenet_v2_model_data);
  static tflite::AllOpsResolver resolver;
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;
  interpreter->AllocateTensors();
  input  = interpreter->input(0);
  output = interpreter->output(0);

  Monitor.printf("[OK] MobileNetV2 加载成功, Arena: %zu/%d bytes\n",
                interpreter->arena_used_bytes(), kTensorArenaSize);
  Monitor.println("开始图像分类...\n");
}

void loop() {
  uint8_t* rawImage = nullptr;
  int imgW = 0, imgH = 0;
  if (!captureImage(&rawImage, &imgW, &imgH)) {
    delay(100);
    return;
  }

  // 缩放并归一化到输入张量
  float scaleX = (float)imgW / kImgW;
  float scaleY = (float)imgH / kImgH;
  for (int y = 0; y < kImgH; y++) {
    for (int x = 0; x < kImgW; x++) {
      int sx = (int)(x * scaleX), sy = (int)(y * scaleY);
      int si = (sy * imgW + sx) * 3;
      int di = (y * kImgW + x) * 3;
      if (input->type == kTfLiteInt8) {
        input->data.int8[di+0] = (int8_t)(rawImage[si+0] - 128);
        input->data.int8[di+1] = (int8_t)(rawImage[si+1] - 128);
        input->data.int8[di+2] = (int8_t)(rawImage[si+2] - 128);
      } else {
        input->data.f[di+0] = (rawImage[si+0] / 127.5f) - 1.0f;
        input->data.f[di+1] = (rawImage[si+1] / 127.5f) - 1.0f;
        input->data.f[di+2] = (rawImage[si+2] / 127.5f) - 1.0f;
      }
    }
  }

  unsigned long t = millis();
  interpreter->Invoke();
  unsigned long inferenceMs = millis() - t;

  // 反量化输出
  static float scores[kNumClasses];
  float scale = output->params.scale;
  int   zp    = output->params.zero_point;
  for (int i = 0; i < kNumClasses; i++) {
    scores[i] = (output->data.int8[i] - zp) * scale;
  }

  // Top-K
  static ClassResult topK[kTopK];
  getTopK(scores, kNumClasses, topK, kTopK);

  Monitor.printf("[Top1] %s (%.1f%%) | 推理: %lums\n",
                kImageNetLabels[topK[0].classId],
                topK[0].score * 100,
                inferenceMs);

  // ===== 发送 Top-1 结果到 Linux MPU =====
  Bridge.call("update_classification",
              kImageNetLabels[topK[0].classId],
              topK[0].score,
              topK[0].classId,
              (int)inferenceMs);

  delay(50);
}