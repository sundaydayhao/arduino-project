/*
 * 实验名称：K 近邻分类器（KNN）
 * 实验目的：使用 KNN 算法对环境状态进行分类
 * 
 * 硬件连接：
 *   DHT11 DATA   → D2
 *   光敏电阻分压 → A0
 * 
 * 分类标签：
 *   0 = 舒适（温度20~26°C，湿度40~60%）
 *   1 = 炎热（温度>28°C）
 *   2 = 潮湿（湿度>70%）
 *   3 = 干燥（湿度<30%）
 *   4 = 昏暗（光照<200）
 * 
 * 所需库：DHT sensor library
 */

#include <DHT.h>

#define DHT_PIN  2
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

const int LDR_PIN = A0;

// ============================================================
// KNN 分类器
// ============================================================
#define NUM_FEATURES  3   // 特征数：温度、湿度、光照
#define NUM_CLASSES   5   // 类别数
#define MAX_SAMPLES   30  // 最大训练样本数（受 SRAM 限制）
#define K_VALUE       3   // K 值

// 类别名称
const char* CLASS_NAMES[] = {
  "舒适", "炎热", "潮湿", "干燥", "昏暗"
};

// 特征归一化范围（根据实际环境调整）
const float FEAT_MIN[NUM_FEATURES] = { 0.0,   0.0,   0.0  };   // 温度、湿度、光照最小值
const float FEAT_MAX[NUM_FEATURES] = { 50.0, 100.0, 1023.0 };  // 温度、湿度、光照最大值

// 训练数据集（预置样本）
// 格式：{温度, 湿度, 光照, 标签}
float trainData[MAX_SAMPLES][NUM_FEATURES] = {
  // 舒适（标签0）
  {22.0, 50.0, 600.0},
  {24.0, 45.0, 700.0},
  {21.0, 55.0, 500.0},
  {23.0, 48.0, 650.0},
  {25.0, 52.0, 580.0},
  // 炎热（标签1）
  {32.0, 55.0, 800.0},
  {35.0, 40.0, 900.0},
  {30.0, 60.0, 750.0},
  {33.0, 45.0, 820.0},
  {38.0, 35.0, 950.0},
  // 潮湿（标签2）
  {25.0, 80.0, 400.0},
  {22.0, 85.0, 350.0},
  {28.0, 75.0, 500.0},
  {24.0, 90.0, 300.0},
  {26.0, 78.0, 450.0},
  // 干燥（标签3）
  {24.0, 20.0, 700.0},
  {26.0, 15.0, 800.0},
  {22.0, 25.0, 600.0},
  {28.0, 18.0, 750.0},
  {25.0, 22.0, 680.0},
  // 昏暗（标签4）
  {22.0, 50.0, 80.0},
  {24.0, 55.0, 100.0},
  {20.0, 45.0, 60.0},
  {23.0, 52.0, 120.0},
  {25.0, 48.0, 90.0},
};

int trainLabels[MAX_SAMPLES] = {
  0, 0, 0, 0, 0,  // 舒适
  1, 1, 1, 1, 1,  // 炎热
  2, 2, 2, 2, 2,  // 潮湿
  3, 3, 3, 3, 3,  // 干燥
  4, 4, 4, 4, 4,  // 昏暗
};

int numSamples = 25;  // 当前训练样本数

// ============================================================
// 特征归一化
// ============================================================
float normalize(float val, int featureIdx) {
  float range = FEAT_MAX[featureIdx] - FEAT_MIN[featureIdx];
  if (range < 1e-6) return 0.0;
  return (val - FEAT_MIN[featureIdx]) / range;
}

// ============================================================
// 欧氏距离计算（归一化后）
// ============================================================
float euclideanDistance(float* a, float* b) {
  float sum = 0.0;
  for (int i = 0; i < NUM_FEATURES; i++) {
    float na = normalize(a[i], i);
    float nb = normalize(b[i], i);
    float diff = na - nb;
    sum += diff * diff;
  }
  return sqrt(sum);
}

// ============================================================
// KNN 预测
// ============================================================
int knnPredict(float* features, float* outConfidence) {
  // 计算到所有训练样本的距离
  float distances[MAX_SAMPLES];
  for (int i = 0; i < numSamples; i++) {
    distances[i] = euclideanDistance(features, trainData[i]);
  }

  // 找到 K 个最近邻（简单选择排序）
  int   kIndices[K_VALUE];
  float kDists[K_VALUE];
  for (int k = 0; k < K_VALUE; k++) kDists[k] = 1e9;

  for (int i = 0; i < numSamples; i++) {
    // 检查是否比当前最远的近邻更近
    int maxIdx = 0;
    for (int k = 1; k < K_VALUE; k++) {
      if (kDists[k] > kDists[maxIdx]) maxIdx = k;
    }
    if (distances[i] < kDists[maxIdx]) {
      kDists[maxIdx]   = distances[i];
      kIndices[maxIdx] = i;
    }
  }

  // 投票
  int votes[NUM_CLASSES] = {0};
  for (int k = 0; k < K_VALUE; k++) {
    votes[trainLabels[kIndices[k]]]++;
  }

  // 找到票数最多的类别
  int bestClass = 0;
  for (int c = 1; c < NUM_CLASSES; c++) {
    if (votes[c] > votes[bestClass]) bestClass = c;
  }

  // 置信度 = 最多票数 / K
  if (outConfidence) *outConfidence = (float)votes[bestClass] / K_VALUE;

  return bestClass;
}

// ============================================================
// 添加新训练样本（在线学习）
// ============================================================
bool addTrainingSample(float* features, int label) {
  if (numSamples >= MAX_SAMPLES) {
    // 替换最旧的同类样本（简单策略）
    for (int i = 0; i < numSamples; i++) {
      if (trainLabels[i] == label) {
        for (int j = 0; j < NUM_FEATURES; j++) {
          trainData[i][j] = features[j];
        }
        return true;
      }
    }
    return false;
  }
  for (int j = 0; j < NUM_FEATURES; j++) {
    trainData[numSamples][j] = features[j];
  }
  trainLabels[numSamples] = label;
  numSamples++;
  return true;
}

// ============================================================
// 全局变量
// ============================================================
unsigned long lastClassifyTime = 0;
const unsigned long CLASSIFY_INTERVAL = 3000;  // 每3秒分类一次

int lastPrediction = -1;
int predictionHistory[5] = {-1, -1, -1, -1, -1};  // 最近5次预测
int historyIdx = 0;

// 获取稳定预测（多数投票）
int getStablePrediction() {
  int votes[NUM_CLASSES] = {0};
  int validCount = 0;
  for (int i = 0; i < 5; i++) {
    if (predictionHistory[i] >= 0) {
      votes[predictionHistory[i]]++;
      validCount++;
    }
  }
  if (validCount == 0) return -1;
  int best = 0;
  for (int c = 1; c < NUM_CLASSES; c++) {
    if (votes[c] > votes[best]) best = c;
  }
  return best;
}

void setup() {
  Serial.begin(9600);
  dht.begin();

  Serial.println(F("========================================"));
  Serial.println(F("   Arduino KNN 环境状态分类器"));
  Serial.println(F("========================================"));
  Serial.print(F("训练样本数: "));
  Serial.println(numSamples);
  Serial.print(F("K 值: "));
  Serial.println(K_VALUE);
  Serial.print(F("特征数: "));
  Serial.println(NUM_FEATURES);
  Serial.println(F("特征: [温度, 湿度, 光照]"));
  Serial.println(F("类别: 舒适/炎热/潮湿/干燥/昏暗"));
  Serial.println(F("----------------------------------------"));
  Serial.println(F("串口命令:"));
  Serial.println(F("  add:<标签>  添加当前读数为训练样本"));
  Serial.println(F("  (标签: 0=舒适 1=炎热 2=潮湿 3=干燥 4=昏暗)"));
  Serial.println(F("----------------------------------------"));
  delay(2000);
}

void loop() {
  unsigned long now = millis();

  // 处理串口命令（在线添加训练样本）
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.startsWith("add:")) {
      int label = cmd.substring(4).toInt();
      if (label >= 0 && label < NUM_CLASSES) {
        float temp = dht.readTemperature();
        float humi = dht.readHumidity();
        float ldr  = analogRead(LDR_PIN);
        if (!isnan(temp) && !isnan(humi)) {
          float features[NUM_FEATURES] = {temp, humi, ldr};
          if (addTrainingSample(features, label)) {
            Serial.print(F("✅ 已添加训练样本: ["));
            Serial.print(temp, 1); Serial.print(F(", "));
            Serial.print(humi, 1); Serial.print(F(", "));
            Serial.print(ldr, 0); Serial.print(F("] → "));
            Serial.println(CLASS_NAMES[label]);
            Serial.print(F("当前训练样本总数: "));
            Serial.println(numSamples);
          } else {
            Serial.println(F("❌ 添加失败（样本库已满）"));
          }
        }
      } else {
        Serial.println(F("❌ 无效标签（0~4）"));
      }
    }
  }

  // 定时分类
  if (now - lastClassifyTime >= CLASSIFY_INTERVAL) {
    lastClassifyTime = now;

    float temp = dht.readTemperature();
    float humi = dht.readHumidity();
    float ldr  = analogRead(LDR_PIN);

    if (isnan(temp) || isnan(humi)) {
      Serial.println(F("[错误] 传感器读取失败"));
      return;
    }

    float features[NUM_FEATURES] = {temp, humi, ldr};
    float confidence = 0.0;
    int prediction = knnPredict(features, &confidence);

    // 更新预测历史
    predictionHistory[historyIdx] = prediction;
    historyIdx = (historyIdx + 1) % 5;

    int stablePred = getStablePrediction();

    // 输出结果
    Serial.print(F("传感器: T="));
    Serial.print(temp, 1);
    Serial.print(F("°C H="));
    Serial.print(humi, 1);
    Serial.print(F("% L="));
    Serial.print(ldr, 0);
    Serial.println();

    Serial.print(F("  KNN预测: "));
    Serial.print(CLASS_NAMES[prediction]);
    Serial.print(F(" (置信度: "));
    Serial.print((int)(confidence * 100));
    Serial.println(F("%)"));

    if (stablePred >= 0) {
      Serial.print(F("  稳定预测: "));
      Serial.println(CLASS_NAMES[stablePred]);
    }

    // 归一化特征值显示
    Serial.print(F("  归一化特征: ["));
    for (int i = 0; i < NUM_FEATURES; i++) {
      Serial.print(normalize(features[i], i), 3);
      if (i < NUM_FEATURES - 1) Serial.print(F(", "));
    }
    Serial.println(F("]"));

    // 状态变化提示
    if (prediction != lastPrediction && lastPrediction >= 0) {
      Serial.print(F("  ⚡ 状态变化: "));
      Serial.print(CLASS_NAMES[lastPrediction]);
      Serial.print(F(" → "));
      Serial.println(CLASS_NAMES[prediction]);
    }
    lastPrediction = prediction;

    Serial.println(F("----------------------------------------"));
  }
}