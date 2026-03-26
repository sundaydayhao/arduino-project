/**
 * AI 推理结果仪表盘 - MCU 端
 * Arduino UNO Q (Qualcomm)
 * 
 * MCU 运行 TFLite 推理（手势/关键词/异常检测），
 * 将推理结果通过 Bridge 发送到 Linux MPU，
 * 在浏览器中实时展示 AI 推理结果和置信度。
 * 
 * 硬件：MPU6050 -> I2C
 */

#include <Arduino.h>
#include <Arduino_RouterBridge.h>
#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// 手势识别模型（从 Edge Impulse 导出）
#include "gesture_model_data.h"

Adafruit_MPU6050 mpu;

// ===== TFLite 配置 =====
constexpr int kArenaSize = 60 * 1024;
static uint8_t tensor_arena[kArenaSize];

const tflite::Model*      model       = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor*             input       = nullptr;
TfLiteTensor*             output      = nullptr;

// ===== 手势标签 =====
const char* GESTURES[] = {"idle", "swipe_up", "swipe_down",
                           "swipe_left", "swipe_right", "circle", "shake"};
const int   GESTURE_COUNT = 7;

// ===== IMU 数据缓冲区 =====
#define SAMPLE_RATE_HZ  100
#define WINDOW_SAMPLES  100
float imuBuffer[WINDOW_SAMPLES * 6];
int   bufIdx = 0;
bool  bufFull = false;

// ===== 推理统计 =====
unsigned long totalInferences = 0;
unsigned long totalInferenceMs = 0;
int gestureCount[7] = {0};

void setup() {
  Bridge.begin();
  Monitor.begin();
  Wire.begin();

  // 初始化 MPU6050
  if (!mpu.begin()) {
    Monitor.println("MPU6050 未找到!");
    while (1) delay(100);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);

  // 初始化 TFLite
  model = tflite::GetModel(g_gesture_model_data);
  static tflite::AllOpsResolver resolver;
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kArenaSize);
  interpreter = &static_interpreter;
  interpreter->AllocateTensors();
  input  = interpreter->input(0);
  output = interpreter->output(0);

  Monitor.println("AI 推理仪表盘已启动");
  Monitor.printf("Arena 使用: %zu / %d bytes\n",
                 interpreter->arena_used_bytes(), kArenaSize);
}

void loop() {
  static unsigned long lastSample = 0;

  // 100Hz 采样
  if (millis() - lastSample >= (1000 / SAMPLE_RATE_HZ)) {
    lastSample = millis();

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    int base = bufIdx * 6;
    imuBuffer[base+0] = a.acceleration.x;
    imuBuffer[base+1] = a.acceleration.y;
    imuBuffer[base+2] = a.acceleration.z;
    imuBuffer[base+3] = g.gyro.x;
    imuBuffer[base+4] = g.gyro.y;
    imuBuffer[base+5] = g.gyro.z;

    bufIdx++;
    if (bufIdx >= WINDOW_SAMPLES) {
      bufIdx = 0;
      bufFull = true;
    }
  }

  // 每窗口运行一次推理
  if (!bufFull) return;
  bufFull = false;

  // 填充输入张量
  for (int i = 0; i < WINDOW_SAMPLES * 6; i++) {
    input->data.f[i] = imuBuffer[i];
  }

  // 推理
  unsigned long t0 = millis();
  interpreter->Invoke();
  unsigned long inferenceMs = millis() - t0;

  // 解析输出
  int   topIdx   = 0;
  float topScore = 0;
  float scores[GESTURE_COUNT];
  for (int i = 0; i < GESTURE_COUNT; i++) {
    scores[i] = output->data.f[i];
    if (scores[i] > topScore) { topScore = scores[i]; topIdx = i; }
  }

  totalInferences++;
  totalInferenceMs += inferenceMs;
  if (topIdx < GESTURE_COUNT) gestureCount[topIdx]++;

  // 通过 Bridge 发送推理结果到 Linux MPU
  Bridge.call("update_inference",
              GESTURES[topIdx],
              topScore,
              scores[0], scores[1], scores[2], scores[3],
              scores[4], scores[5], scores[6],
              (long)inferenceMs,
              (long)totalInferences,
              (float)totalInferenceMs / totalInferences);

  Monitor.printf("[%lu] %s (%.0f%%) %lums\n",
                 totalInferences, GESTURES[topIdx],
                 topScore * 100, inferenceMs);
}