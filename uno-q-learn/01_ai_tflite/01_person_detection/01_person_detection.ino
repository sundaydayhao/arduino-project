/**
 * TensorFlow Lite 人体检测例程
 * Arduino UNO Q (Qualcomm)
 * 
 * 利用 Qualcomm Cortex-M33 @ 400MHz 的算力，
 * 运行 MobileNet V1 量化模型实现实时人体检测。
 * 
 * 硬件：
 *   OV2640 摄像头模块 (SPI/DVP)
 *   或 HM01B0 超低功耗摄像头
 * 
 * 模型：person_detect_model_data (96x96 灰度, INT8 量化)
 * 性能：~200ms/帧 (Cortex-M33 @ 400MHz)
 * 
 * 依赖库：
 *   Arduino_TensorFlowLite (需安装)
 *   Arduino_OV767X 或 Arducam (需安装)
 */

#include <Arduino_RouterBridge.h>  // Bridge MCU↔Linux MPU
#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_log.h>
#include <tensorflow/lite/schema/schema_generated.h>

// 摄像头库（根据实际模块选择）
#include "camera_provider.h"   // 摄像头抽象层

// 内置的 person_detect 模型（量化 INT8，约 300KB）
#include "person_detect_model_data.h"

// ===== TFLite 配置 =====
// Qualcomm Cortex-M33 有 512KB SRAM，分配 300KB 给 TFLite
constexpr int kTensorArenaSize = 300 * 1024;
static uint8_t tensor_arena[kTensorArenaSize];

// 模型输入尺寸
constexpr int kNumRows    = 96;
constexpr int kNumCols    = 96;
constexpr int kNumChannels = 1;  // 灰度图

// 检测阈值
constexpr float kPersonThreshold    = 0.7f;
constexpr float kNoPersonThreshold  = 0.7f;

// TFLite 对象
const tflite::Model*         model       = nullptr;
tflite::MicroInterpreter*    interpreter = nullptr;
TfLiteTensor*                input       = nullptr;
TfLiteTensor*                output      = nullptr;

// 统计
int personCount   = 0;
int noPersonCount = 0;
int totalFrames   = 0;
unsigned long totalInferenceMs = 0;

// ===== 初始化 TFLite =====
bool initTFLite() {
  // 加载模型
  model = tflite::GetModel(g_person_detect_model_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.printf("模型版本不匹配: %d vs %d\n",
                  model->version(), TFLITE_SCHEMA_VERSION);
    return false;
  }

  // 注册所有算子（生产环境可只注册需要的算子节省内存）
  static tflite::AllOpsResolver resolver;

  // 创建解释器
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  // 分配张量内存
  TfLiteStatus status = interpreter->AllocateTensors();
  if (status != kTfLiteOk) {
    Serial.println("AllocateTensors 失败!");
    return false;
  }

  // 获取输入/输出张量
  input  = interpreter->input(0);
  output = interpreter->output(0);

  Serial.printf("模型加载成功!\n");
  Serial.printf("  输入: [%d, %d, %d] 类型: %d\n",
                input->dims->data[1], input->dims->data[2],
                input->dims->data[3], input->type);
  Serial.printf("  输出: [%d] 类型: %d\n",
                output->dims->data[1], output->type);
  Serial.printf("  Tensor Arena 使用: %zu / %d bytes\n",
                interpreter->arena_used_bytes(), kTensorArenaSize);
  return true;
}

// ===== 运行推理 =====
struct DetectionResult {
  float personScore;
  float noPersonScore;
  bool  personDetected;
  unsigned long inferenceMs;
};

DetectionResult runInference(const uint8_t* imageData) {
  DetectionResult result = {0};

  // 将图像数据复制到输入张量
  // INT8 量化：将 [0,255] 映射到 [-128, 127]
  if (input->type == kTfLiteInt8) {
    int8_t* inputData = input->data.int8;
    for (int i = 0; i < kNumRows * kNumCols * kNumChannels; i++) {
      inputData[i] = (int8_t)(imageData[i] - 128);
    }
  } else if (input->type == kTfLiteUInt8) {
    memcpy(input->data.uint8, imageData, kNumRows * kNumCols * kNumChannels);
  } else if (input->type == kTfLiteFloat32) {
    float* inputData = input->data.f;
    for (int i = 0; i < kNumRows * kNumCols * kNumChannels; i++) {
      inputData[i] = imageData[i] / 255.0f;
    }
  }

  // 执行推理
  unsigned long t = millis();
  TfLiteStatus status = interpreter->Invoke();
  result.inferenceMs = millis() - t;

  if (status != kTfLiteOk) {
    Serial.println("推理失败!");
    return result;
  }

  // 解析输出（person_detect 模型输出 2 个类别）
  // output[0] = no_person 分数, output[1] = person 分数
  if (output->type == kTfLiteInt8) {
    // 反量化
    float scale     = output->params.scale;
    int   zeroPoint = output->params.zero_point;
    result.noPersonScore = (output->data.int8[0] - zeroPoint) * scale;
    result.personScore   = (output->data.int8[1] - zeroPoint) * scale;
  } else if (output->type == kTfLiteFloat32) {
    result.noPersonScore = output->data.f[0];
    result.personScore   = output->data.f[1];
  }

  result.personDetected = (result.personScore > kPersonThreshold);
  return result;
}

// ===== 图像预处理（缩放到 96x96 灰度）=====
void preprocessImage(const uint8_t* src, int srcW, int srcH,
                     uint8_t* dst, int dstW, int dstH) {
  float scaleX = (float)srcW / dstW;
  float scaleY = (float)srcH / dstH;

  for (int y = 0; y < dstH; y++) {
    for (int x = 0; x < dstW; x++) {
      int srcX = (int)(x * scaleX);
      int srcY = (int)(y * scaleY);
      // 假设源图像为 RGB888，转灰度
      int idx = (srcY * srcW + srcX) * 3;
      dst[y * dstW + x] = (uint8_t)(
        0.299f * src[idx] + 0.587f * src[idx+1] + 0.114f * src[idx+2]);
    }
  }
}

void setup() {
  // 初始化 Bridge（MCU ↔ Linux MPU 通信）
  Bridge.begin();
  Monitor.begin();

  Monitor.println("\n========================================");
  Monitor.println("  Arduino UNO Q - TFLite 人体检测");
  Monitor.println("  MCU: TFLite 推理 | MPU: 日志+仪表盘");
  Monitor.println("========================================\n");

  // 初始化摄像头
  Monitor.println("初始化摄像头...");
  if (!initCamera()) {
    Monitor.println("❌ 摄像头初始化失败!");
    Monitor.println("  请检查 OV2640/HM01B0 接线");
    while (1) delay(1000);
  }
  Monitor.println("✅ 摄像头初始化成功");

  // 初始化 TFLite
  Monitor.println("加载 TFLite 模型...");
  if (!initTFLite()) {
    Monitor.println("❌ TFLite 初始化失败!");
    while (1) delay(1000);
  }
  Monitor.println("✅ TFLite 模型加载成功\n");

  Monitor.println("开始实时人体检测...");
  Monitor.println("格式: [帧号] 结果 | 人体:xx% 无人:xx% | 推理:xxms\n");
}

void loop() {
  // 获取摄像头图像
  uint8_t* rawImage = nullptr;
  int imgW = 0, imgH = 0;

  if (!captureImage(&rawImage, &imgW, &imgH)) {
    Serial.println("摄像头捕获失败");
    delay(100);
    return;
  }

  // 预处理：缩放到 96x96 灰度
  static uint8_t processedImage[kNumRows * kNumCols];
  preprocessImage(rawImage, imgW, imgH, processedImage, kNumRows, kNumCols);

  // 运行推理
  DetectionResult result = runInference(processedImage);

  totalFrames++;
  totalInferenceMs += result.inferenceMs;
  if (result.personDetected) personCount++;
  else noPersonCount++;

  // 输出结果到串口监视器
  Monitor.printf("[%4d] %s | 人体:%5.1f%% 无人:%5.1f%% | 推理:%3lums | FPS:%.1f\n",
                totalFrames,
                result.personDetected ? "检测到人体" : "无人",
                result.personScore   * 100,
                result.noPersonScore * 100,
                result.inferenceMs,
                1000.0f / result.inferenceMs);

  // ===== 发送推理结果到 Linux MPU =====
  // MPU 端 main.py 的 update_detection() 函数接收此数据
  Bridge.call("update_detection",
              result.personScore,
              result.noPersonScore,
              (int)result.inferenceMs);

  // 每 100 帧输出统计
  if (totalFrames % 100 == 0) {
    Monitor.printf("\n===== 统计 (共%d帧) =====\n", totalFrames);
    Monitor.printf("  检测到人体: %d 次 (%.1f%%)\n",
                  personCount, 100.0f * personCount / totalFrames);
    Monitor.printf("  平均推理时间: %.1f ms\n",
                  (float)totalInferenceMs / totalFrames);
    Monitor.printf("  平均 FPS: %.1f\n\n",
                  1000.0f * totalFrames / totalInferenceMs);
  }
}