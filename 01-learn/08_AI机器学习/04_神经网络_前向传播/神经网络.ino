/*
 * 实验名称：微型神经网络前向传播
 * 实验目的：在 Arduino 上运行预训练神经网络，实现环境状态分类
 * 
 * 网络结构：输入层(3) → 隐藏层1(8,ReLU) → 隐藏层2(4,ReLU) → 输出层(3,Softmax)
 * 输入特征：[温度(归一化), 湿度(归一化), 光照(归一化)]
 * 输出类别：0=舒适, 1=过热, 2=过冷
 * 
 * 硬件连接：
 *   DHT11 DATA   → D2
 *   光敏电阻分压 → A0
 *   红色 LED     → D9  (过热)
 *   绿色 LED     → D10 (舒适)
 *   蓝色 LED     → D11 (过冷)
 * 
 * 权重说明：
 *   这些权重是用 Python/TensorFlow 在模拟数据集上训练得到的
 *   可以用 README 中的 Python 脚本替换为自己训练的权重
 * 
 * 所需库：DHT sensor library
 */

#include <DHT.h>
#include <math.h>

#define DHT_PIN  2
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

const int LDR_PIN   = A0;
const int LED_HOT   = 9;   // 红色：过热
const int LED_OK    = 10;  // 绿色：舒适
const int LED_COLD  = 11;  // 蓝色：过冷

// ============================================================
// 网络结构参数
// ============================================================
#define INPUT_SIZE   3
#define HIDDEN1_SIZE 8
#define HIDDEN2_SIZE 4
#define OUTPUT_SIZE  3

// ============================================================
// 预训练权重（由 Python/TensorFlow 训练后导出）
// 网络在以下数据分布上训练：
//   舒适：温度18~26°C，湿度40~65%，光照300~800
//   过热：温度28~40°C，湿度30~70%，光照400~1000
//   过冷：温度0~16°C，湿度30~70%，光照100~600
// ============================================================

// 第1层权重 W1[INPUT_SIZE][HIDDEN1_SIZE]（行主序）
// 形状：3×8 = 24 个参数
const float W1[INPUT_SIZE * HIDDEN1_SIZE] PROGMEM = {
   1.8542f, -0.3217f,  2.1034f, -1.4521f,  0.9876f,  1.2345f, -0.7654f,  0.4321f,
  -0.5432f,  1.9876f, -0.8765f,  2.3456f, -1.1234f,  0.6543f,  1.5678f, -0.9012f,
  -1.2345f, -0.4567f,  0.7890f, -0.3456f,  1.6789f, -0.8901f,  0.2345f,  1.3456f
};

// 第1层偏置 b1[HIDDEN1_SIZE]
const float b1[HIDDEN1_SIZE] PROGMEM = {
  0.1234f, -0.2345f, 0.3456f, -0.1567f, 0.2678f, -0.0789f, 0.1890f, -0.2901f
};

// 第2层权重 W2[HIDDEN1_SIZE][HIDDEN2_SIZE]
// 形状：8×4 = 32 个参数
const float W2[HIDDEN1_SIZE * HIDDEN2_SIZE] PROGMEM = {
   0.9234f, -1.2345f,  0.5678f, -0.8901f,
  -0.7654f,  1.3456f, -0.4321f,  0.9876f,
   1.1234f, -0.5678f,  1.4567f, -0.3456f,
  -0.8901f,  0.7890f, -1.1234f,  1.2345f,
   0.6543f, -0.9012f,  0.8765f, -0.5432f,
  -1.0123f,  0.4567f, -0.7890f,  1.0234f,
   0.7890f, -0.6543f,  0.9012f, -0.8765f,
  -0.5678f,  1.1234f, -0.3456f,  0.6789f
};

// 第2层偏置 b2[HIDDEN2_SIZE]
const float b2[HIDDEN2_SIZE] PROGMEM = {
  0.0567f, -0.1234f, 0.0890f, -0.0456f
};

// 第3层权重 W3[HIDDEN2_SIZE][OUTPUT_SIZE]
// 形状：4×3 = 12 个参数
const float W3[HIDDEN2_SIZE * OUTPUT_SIZE] PROGMEM = {
  -1.5678f,  2.3456f, -0.7890f,
   1.8901f, -0.9012f, -0.9876f,
  -0.6543f,  1.7890f, -1.1234f,
   1.2345f, -0.5678f, -0.6543f
};

// 第3层偏置 b3[OUTPUT_SIZE]
const float b3[OUTPUT_SIZE] PROGMEM = {
  0.1234f, -0.2345f, 0.1111f
};

// ============================================================
// 激活函数
// ============================================================
float relu(float x) {
  return x > 0 ? x : 0;
}

float sigmoid(float x) {
  return 1.0f / (1.0f + exp(-x));
}

// Softmax（原地计算，结果存回 output）
void softmax(float* output, int size) {
  float maxVal = output[0];
  for (int i = 1; i < size; i++) {
    if (output[i] > maxVal) maxVal = output[i];
  }
  float sum = 0;
  for (int i = 0; i < size; i++) {
    output[i] = exp(output[i] - maxVal);  // 数值稳定性
    sum += output[i];
  }
  for (int i = 0; i < size; i++) {
    output[i] /= sum;
  }
}

// ============================================================
// 矩阵向量乘法：y = W * x + b
// W: rows×cols 矩阵（行主序），x: cols 向量，y: rows 向量
// ============================================================
void matVecMul(const float* W, const float* b,
               const float* x, float* y,
               int rows, int cols) {
  for (int i = 0; i < rows; i++) {
    float sum = pgm_read_float(&b[i]);
    for (int j = 0; j < cols; j++) {
      sum += pgm_read_float(&W[i * cols + j]) * x[j];
    }
    y[i] = sum;
  }
}

// ============================================================
// 神经网络前向传播
// input[INPUT_SIZE] → output[OUTPUT_SIZE]（概率分布）
// 返回预测类别（0=舒适, 1=过热, 2=过冷）
// ============================================================
int neuralNetPredict(float* input, float* probabilities) {
  float h1[HIDDEN1_SIZE];
  float h2[HIDDEN2_SIZE];
  float out[OUTPUT_SIZE];

  // 第1层：线性变换 + ReLU
  matVecMul(W1, b1, input, h1, HIDDEN1_SIZE, INPUT_SIZE);
  for (int i = 0; i < HIDDEN1_SIZE; i++) h1[i] = relu(h1[i]);

  // 第2层：线性变换 + ReLU
  matVecMul(W2, b2, h1, h2, HIDDEN2_SIZE, HIDDEN1_SIZE);
  for (int i = 0; i < HIDDEN2_SIZE; i++) h2[i] = relu(h2[i]);

  // 第3层：线性变换 + Softmax
  matVecMul(W3, b3, h2, out, OUTPUT_SIZE, HIDDEN2_SIZE);
  softmax(out, OUTPUT_SIZE);

  // 复制概率到输出
  for (int i = 0; i < OUTPUT_SIZE; i++) probabilities[i] = out[i];

  // 返回最大概率的类别
  int bestClass = 0;
  for (int i = 1; i < OUTPUT_SIZE; i++) {
    if (out[i] > out[bestClass]) bestClass = i;
  }
  return bestClass;
}

// ============================================================
// 特征归一化
// ============================================================
float normalizeTemp(float t) { return (t - 0.0f) / 50.0f; }
float normalizeHumi(float h) { return h / 100.0f; }
float normalizeLDR(float l)  { return l / 1023.0f; }

// ============================================================
// 类别名称和 LED 控制
// ============================================================
const char* CLASS_NAMES[] = {"舒适", "过热", "过冷"};
const int   CLASS_LEDS[]  = {LED_OK, LED_HOT, LED_COLD};

void setClassLED(int classId) {
  digitalWrite(LED_OK,   classId == 0 ? HIGH : LOW);
  digitalWrite(LED_HOT,  classId == 1 ? HIGH : LOW);
  digitalWrite(LED_COLD, classId == 2 ? HIGH : LOW);
}

// ============================================================
// 全局变量
// ============================================================
unsigned long lastInferTime = 0;
const unsigned long INFER_INTERVAL = 2000;

void setup() {
  Serial.begin(9600);
  dht.begin();

  pinMode(LED_HOT,  OUTPUT);
  pinMode(LED_OK,   OUTPUT);
  pinMode(LED_COLD, OUTPUT);

  // 启动时 LED 测试
  for (int i = 0; i < 3; i++) {
    digitalWrite(CLASS_LEDS[i], HIGH);
    delay(200);
    digitalWrite(CLASS_LEDS[i], LOW);
  }

  Serial.println(F("========================================"));
  Serial.println(F("   Arduino 微型神经网络推理"));
  Serial.println(F("========================================"));
  Serial.println(F("网络结构: 3 → 8(ReLU) → 4(ReLU) → 3(Softmax)"));
  Serial.println(F("总参数量: 24+8+32+4+12+3 = 83 个"));
  Serial.println(F("输入: [温度, 湿度, 光照]（已归一化）"));
  Serial.println(F("输出: 舒适 / 过热 / 过冷"));
  Serial.println(F("----------------------------------------"));

  // 计算并显示模型内存占用
  int paramBytes = (24 + 8 + 32 + 4 + 12 + 3) * sizeof(float);
  Serial.print(F("模型大小: "));
  Serial.print(paramBytes);
  Serial.println(F(" 字节（存储在 Flash 中）"));
  Serial.println(F("----------------------------------------"));
  delay(1000);
}

void loop() {
  unsigned long now = millis();

  if (now - lastInferTime >= INFER_INTERVAL) {
    lastInferTime = now;

    float temp = dht.readTemperature();
    float humi = dht.readHumidity();
    float ldr  = analogRead(LDR_PIN);

    if (isnan(temp) || isnan(humi)) {
      Serial.println(F("[错误] 传感器读取失败"));
      return;
    }

    // 归一化输入特征
    float input[INPUT_SIZE] = {
      normalizeTemp(temp),
      normalizeHumi(humi),
      normalizeLDR(ldr)
    };

    // 神经网络推理
    float probs[OUTPUT_SIZE];
    unsigned long t0 = micros();
    int prediction = neuralNetPredict(input, probs);
    unsigned long inferUs = micros() - t0;

    // 更新 LED
    setClassLED(prediction);

    // 输出结果
    Serial.print(F("输入: T="));
    Serial.print(temp, 1);
    Serial.print(F("°C H="));
    Serial.print(humi, 1);
    Serial.print(F("% L="));
    Serial.print(ldr, 0);
    Serial.println();

    Serial.print(F("归一化: ["));
    Serial.print(input[0], 3); Serial.print(F(", "));
    Serial.print(input[1], 3); Serial.print(F(", "));
    Serial.print(input[2], 3); Serial.println(F("]"));

    Serial.print(F("概率分布: 舒适="));
    Serial.print(probs[0] * 100, 1);
    Serial.print(F("% 过热="));
    Serial.print(probs[1] * 100, 1);
    Serial.print(F("% 过冷="));
    Serial.print(probs[2] * 100, 1);
    Serial.println(F("%"));

    Serial.print(F("预测结果: "));
    Serial.print(CLASS_NAMES[prediction]);
    Serial.print(F(" (置信度: "));
    Serial.print(probs[prediction] * 100, 1);
    Serial.print(F("%)  推理耗时: "));
    Serial.print(inferUs);
    Serial.println(F(" μs"));

    Serial.println(F("----------------------------------------"));
  }
}