/*
 * 实验名称：TensorFlow Lite 手势识别
 * 实验目的：使用 TFLite Micro 在 Arduino Nano 33 BLE Sense 上识别手势
 * 
 * ⚠️ 硬件要求：Arduino Nano 33 BLE Sense（不支持 UNO）
 * 
 * 所需库：
 *   - Arduino_TensorFlowLite (>= 2.4.0)
 *   - Arduino_LSM9DS1 (>= 1.1.0)
 * 
 * 手势类别：
 *   0 = 上挥 (Swipe Up)
 *   1 = 下挥 (Swipe Down)
 *   2 = 左挥 (Swipe Left)
 *   3 = 右挥 (Swipe Right)
 * 
 * 工作流程：
 *   1. 持续读取 IMU（加速度计 + 陀螺仪）
 *   2. 检测运动触发（加速度超过阈值）
 *   3. 采集 119 个时间步的 6 轴数据
 *   4. 归一化后送入 TFLite 模型推理
 *   5. 输出手势类别和置信度
 */

#include <Arduino_LSM9DS1.h>
#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>

#include "gesture_model.h"

// ============================================================
// TFLite 配置
// ============================================================
// 张量 Arena 大小（根据模型大小调整）
constexpr int TENSOR_ARENA_SIZE = 8 * 1024;  // 8KB
alignas(16) uint8_t tensor_arena[TENSOR_ARENA_SIZE];

// TFLite 对象
const tflite::Model*         tflModel     = nullptr;
tflite::MicroInterpreter*    interpreter  = nullptr;
TfLiteTensor*                inputTensor  = nullptr;
TfLiteTensor*                outputTensor = nullptr;
tflite::AllOpsResolver       resolver;

// ============================================================
// IMU 数据采集参数
// ============================================================
const int   NUM_SAMPLES    = 119;   // 每次手势采集的样本数
const int   NUM_AXES       = 6;     // 6轴：ax,ay,az,gx,gy,gz
const float ACCEL_THRESHOLD = 2.5f; // 触发采集的加速度阈值（g）

float imuBuffer[NUM_SAMPLES][NUM_AXES];
int   samplesRead = 0;
bool  collecting  = false;

// ============================================================
// 推理结果
// ============================================================
const float CONFIDENCE_THRESHOLD = 0.7f;  // 置信度阈值

// ============================================================
// 初始化 TFLite
// ============================================================
bool initTFLite() {
  // 加载模型
  tflModel = tflite::GetModel(gesture_model);
  if (tflModel->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println(F("[TFLite] 模型版本不匹配！"));
    return false;
  }

  // 创建解释器
  static tflite::MicroInterpreter static_interpreter(
    tflModel, resolver, tensor_arena, TENSOR_ARENA_SIZE
  );
  interpreter = &static_interpreter;

  // 分配张量内存
  TfLiteStatus allocStatus = interpreter->AllocateTensors();
  if (allocStatus != kTfLiteOk) {
    Serial.println(F("[TFLite] 张量内存分配失败！"));
    return false;
  }

  // 获取输入/输出张量
  inputTensor  = interpreter->input(0);
  outputTensor = interpreter->output(0);

  Serial.print(F("[TFLite] 输入张量形状: ["));
  for (int i = 0; i < inputTensor->dims->size; i++) {
    Serial.print(inputTensor->dims->data[i]);
    if (i < inputTensor->dims->size - 1) Serial.print(F(", "));
  }
  Serial.println(F("]"));

  Serial.print(F("[TFLite] 输出张量形状: ["));
  for (int i = 0; i < outputTensor->dims->size; i++) {
    Serial.print(outputTensor->dims->data[i]);
    if (i < outputTensor->dims->size - 1) Serial.print(F(", "));
  }
  Serial.println(F("]"));

  Serial.print(F("[TFLite] Arena 使用: "));
  Serial.print(interpreter->arena_used_bytes());
  Serial.println(F(" 字节"));

  return true;
}

// ============================================================
// 运行推理
// ============================================================
int runInference(float confidence[]) {
  // 将 IMU 数据填入输入张量（归一化到 -1~1）
  float* inputData = inputTensor->data.f;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    for (int j = 0; j < NUM_AXES; j++) {
      // 加速度计：±4g 范围归一化；陀螺仪：±2000dps 归一化
      float scale = (j < 3) ? 4.0f : 2000.0f;
      inputData[i * NUM_AXES + j] = imuBuffer[i][j] / scale;
    }
  }

  // 执行推理
  unsigned long t0 = micros();
  TfLiteStatus invokeStatus = interpreter->Invoke();
  unsigned long inferUs = micros() - t0;

  if (invokeStatus != kTfLiteOk) {
    Serial.println(F("[TFLite] 推理失败！"));
    return -1;
  }

  // 读取输出概率
  float* outputData = outputTensor->data.f;
  int bestClass = 0;
  for (int i = 0; i < NUM_GESTURES; i++) {
    confidence[i] = outputData[i];
    if (confidence[i] > confidence[bestClass]) bestClass = i;
  }

  Serial.print(F("[推理耗时: "));
  Serial.print(inferUs / 1000.0f, 1);
  Serial.println(F(" ms]"));

  return bestClass;
}

// ============================================================
// 打印推理结果
// ============================================================
void printResult(int gesture, float confidence[]) {
  Serial.println(F("--- 手势识别结果 ---"));
  for (int i = 0; i < NUM_GESTURES; i++) {
    Serial.print(F("  "));
    Serial.print(GESTURE_LABELS[i]);
    Serial.print(F(": "));
    Serial.print(confidence[i] * 100, 1);
    Serial.print(F("%"));
    if (i == gesture) Serial.print(F(" ← 预测"));
    Serial.println();
  }

  if (confidence[gesture] >= CONFIDENCE_THRESHOLD) {
    Serial.print(F("✅ 识别结果: "));
    Serial.println(GESTURE_LABELS[gesture]);
  } else {
    Serial.println(F("❓ 置信度不足，无法确定手势"));
  }
  Serial.println(F("--------------------"));
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println(F("========================================"));
  Serial.println(F("  TFLite 手势识别 - Arduino Nano 33 BLE"));
  Serial.println(F("========================================"));

  // 初始化 IMU
  if (!IMU.begin()) {
    Serial.println(F("[错误] IMU 初始化失败！"));
    while (true);
  }
  Serial.print(F("加速度计采样率: "));
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(F(" Hz"));
  Serial.print(F("陀螺仪采样率: "));
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println(F(" Hz"));

  // 初始化 TFLite
  Serial.println(F("正在加载 TFLite 模型..."));
  if (!initTFLite()) {
    Serial.println(F("[错误] TFLite 初始化失败！"));
    while (true);
  }
  Serial.println(F("✅ TFLite 模型加载成功"));
  Serial.println(F("----------------------------------------"));
  Serial.println(F("准备就绪！请做手势（上挥/下挥/左挥/右挥）"));
  Serial.println(F("（快速移动 Arduino 板触发识别）"));
}

void loop() {
  float ax, ay, az, gx, gy, gz;

  if (!collecting) {
    // 等待运动触发
    if (IMU.accelerationAvailable()) {
      IMU.readAcceleration(ax, ay, az);
      float magnitude = sqrt(ax*ax + ay*ay + az*az);

      if (magnitude > ACCEL_THRESHOLD) {
        collecting  = true;
        samplesRead = 0;
        Serial.println(F("⚡ 检测到运动，开始采集..."));
      }
    }
  } else {
    // 采集 IMU 数据
    if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
      IMU.readAcceleration(ax, ay, az);
      IMU.readGyroscope(gx, gy, gz);

      imuBuffer[samplesRead][0] = ax;
      imuBuffer[samplesRead][1] = ay;
      imuBuffer[samplesRead][2] = az;
      imuBuffer[samplesRead][3] = gx;
      imuBuffer[samplesRead][4] = gy;
      imuBuffer[samplesRead][5] = gz;

      samplesRead++;

      if (samplesRead == NUM_SAMPLES) {
        // 采集完成，运行推理
        collecting = false;
        Serial.print(F("采集完成（"));
        Serial.print(NUM_SAMPLES);
        Serial.println(F(" 个样本），开始推理..."));

        float confidence[NUM_GESTURES];
        int gesture = runInference(confidence);

        if (gesture >= 0) {
          printResult(gesture, confidence);
        }
      }
    }
  }
}