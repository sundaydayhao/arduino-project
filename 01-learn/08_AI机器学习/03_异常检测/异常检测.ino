/*
 * 实验名称：传感器数据异常检测
 * 实验目的：使用 Z-Score 和移动窗口统计方法检测传感器异常
 * 
 * 硬件连接：
 *   DHT11 DATA → D2
 *   蜂鸣器     → D8
 *   红色 LED   → D13（220Ω）
 * 
 * 算法说明：
 *   1. 预热阶段：收集初始样本，建立基线统计量（均值、标准差）
 *   2. 检测阶段：对每个新样本计算 Z-Score
 *   3. 若 |Z-Score| > 阈值，则判定为异常并报警
 *   4. 使用指数加权移动平均（EWMA）动态更新基线
 * 
 * 所需库：DHT sensor library
 */

#include <DHT.h>

#define DHT_PIN  2
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

const int BUZZER_PIN = 8;
const int LED_PIN    = 13;

// ============================================================
// 移动窗口统计器
// ============================================================
#define WINDOW_SIZE 20  // 滑动窗口大小

struct WindowStats {
  float  buffer[WINDOW_SIZE];
  int    head;
  int    count;
  double sum;
  double sumSq;

  WindowStats() : head(0), count(0), sum(0), sumSq(0) {
    for (int i = 0; i < WINDOW_SIZE; i++) buffer[i] = 0;
  }

  void push(float val) {
    if (count == WINDOW_SIZE) {
      // 移除最旧的值
      float old = buffer[head];
      sum   -= old;
      sumSq -= (double)old * old;
    } else {
      count++;
    }
    buffer[head] = val;
    head = (head + 1) % WINDOW_SIZE;
    sum   += val;
    sumSq += (double)val * val;
  }

  float getMean() {
    if (count == 0) return 0;
    return (float)(sum / count);
  }

  float getStdDev() {
    if (count < 2) return 0;
    double mean = sum / count;
    double variance = sumSq / count - mean * mean;
    if (variance < 0) variance = 0;
    return (float)sqrt(variance);
  }

  float getZScore(float val) {
    float sigma = getStdDev();
    if (sigma < 0.01) return 0;  // 标准差太小，无法计算
    return (val - getMean()) / sigma;
  }

  bool isReady() {
    return count >= WINDOW_SIZE / 2;  // 至少半窗口数据
  }
};

// ============================================================
// 指数加权移动平均（EWMA）
// 用于动态跟踪基线，适应缓慢的环境变化
// ============================================================
struct EWMA {
  float value;
  float alpha;  // 平滑系数（0~1，越小越平滑）
  bool  initialized;

  EWMA(float a = 0.1) : value(0), alpha(a), initialized(false) {}

  float update(float x) {
    if (!initialized) {
      value = x;
      initialized = true;
    } else {
      value = alpha * x + (1.0 - alpha) * value;
    }
    return value;
  }
};

// ============================================================
// 异常检测器
// ============================================================
struct AnomalyDetector {
  WindowStats stats;
  EWMA        baseline;
  float       zThreshold;    // Z-Score 阈值
  float       deltaThreshold; // 突变阈值（相邻两次差值）
  float       lastValue;
  int         anomalyCount;
  int         totalCount;

  AnomalyDetector(float zThr = 2.5, float deltaThr = 5.0)
    : baseline(0.05), zThreshold(zThr), deltaThreshold(deltaThr),
      lastValue(0), anomalyCount(0), totalCount(0) {}

  // 检测结果
  enum Result {
    NORMAL    = 0,
    Z_ANOMALY = 1,  // Z-Score 异常
    DELTA_ANOMALY = 2,  // 突变异常
    BOTH_ANOMALY  = 3   // 两种异常同时触发
  };

  Result detect(float val) {
    totalCount++;
    bool zAnom = false, deltaAnom = false;

    // Z-Score 检测
    if (stats.isReady()) {
      float z = stats.getZScore(val);
      if (abs(z) > zThreshold) zAnom = true;
    }

    // 突变检测（相邻差值）
    if (totalCount > 1) {
      float delta = abs(val - lastValue);
      if (delta > deltaThreshold) deltaAnom = true;
    }

    // 更新统计（只用正常值更新基线，防止异常污染模型）
    if (!zAnom && !deltaAnom) {
      stats.push(val);
      baseline.update(val);
    }

    lastValue = val;

    if (zAnom && deltaAnom) { anomalyCount++; return BOTH_ANOMALY; }
    if (zAnom)              { anomalyCount++; return Z_ANOMALY; }
    if (deltaAnom)          { anomalyCount++; return DELTA_ANOMALY; }
    return NORMAL;
  }

  float getAnomalyRate() {
    if (totalCount == 0) return 0;
    return (float)anomalyCount / totalCount * 100.0;
  }
};

// ============================================================
// 全局变量
// ============================================================
AnomalyDetector tempDetector(2.5, 3.0);  // 温度：Z阈值2.5，突变阈值3°C
AnomalyDetector humiDetector(2.5, 10.0); // 湿度：Z阈值2.5，突变阈值10%

unsigned long lastSampleTime = 0;
const unsigned long SAMPLE_INTERVAL = 5000;  // 每5秒采样

int sampleCount = 0;
bool warmupDone = false;
const int WARMUP_SAMPLES = 10;  // 预热样本数

// 报警状态
bool isAlarming = false;
unsigned long alarmStartTime = 0;
const unsigned long ALARM_DURATION = 3000;  // 报警持续3秒

void triggerAlarm(const char* reason) {
  isAlarming = true;
  alarmStartTime = millis();
  digitalWrite(LED_PIN, HIGH);
  Serial.print(F("  🚨 异常报警: "));
  Serial.println(reason);
}

void updateAlarm() {
  if (isAlarming && millis() - alarmStartTime >= ALARM_DURATION) {
    isAlarming = false;
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);
  }
  if (isAlarming) {
    // 间歇蜂鸣
    static unsigned long lastBeep = 0;
    static bool beepOn = false;
    if (millis() - lastBeep >= 300) {
      lastBeep = millis();
      beepOn = !beepOn;
      if (beepOn) tone(BUZZER_PIN, 2500, 250);
    }
  }
}

const char* getResultName(AnomalyDetector::Result r) {
  switch (r) {
    case AnomalyDetector::NORMAL:       return "正常";
    case AnomalyDetector::Z_ANOMALY:    return "Z-Score异常";
    case AnomalyDetector::DELTA_ANOMALY: return "突变异常";
    case AnomalyDetector::BOTH_ANOMALY: return "复合异常";
    default: return "未知";
  }
}

void setup() {
  Serial.begin(9600);
  dht.begin();

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN,    OUTPUT);

  Serial.println(F("========================================"));
  Serial.println(F("   Arduino 传感器异常检测系统"));
  Serial.println(F("========================================"));
  Serial.println(F("算法: Z-Score + 突变检测 + EWMA基线"));
  Serial.println(F("温度阈值: Z>2.5 或 突变>3°C"));
  Serial.println(F("湿度阈值: Z>2.5 或 突变>10%"));
  Serial.print(F("预热样本数: "));
  Serial.println(WARMUP_SAMPLES);
  Serial.println(F("----------------------------------------"));
  Serial.println(F("串口命令:"));
  Serial.println(F("  inject  注入一个模拟异常值（测试用）"));
  Serial.println(F("  stats   显示统计摘要"));
  Serial.println(F("----------------------------------------"));
  delay(2000);
  Serial.println(F("开始预热，请保持环境稳定..."));
}

void loop() {
  unsigned long now = millis();

  // 处理串口命令
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "inject") {
      // 注入模拟异常（温度突然+15°C）
      float fakeTemp = tempDetector.stats.getMean() + 15.0;
      Serial.print(F("💉 注入模拟异常温度: "));
      Serial.print(fakeTemp, 1);
      Serial.println(F("°C"));
      auto r = tempDetector.detect(fakeTemp);
      if (r != AnomalyDetector::NORMAL) {
        triggerAlarm(getResultName(r));
      }
    } else if (cmd == "stats") {
      Serial.println(F("===== 统计摘要 ====="));
      Serial.print(F("总样本数: ")); Serial.println(sampleCount);
      Serial.print(F("温度均值: ")); Serial.print(tempDetector.stats.getMean(), 2); Serial.println(F("°C"));
      Serial.print(F("温度标准差: ")); Serial.print(tempDetector.stats.getStdDev(), 3); Serial.println(F("°C"));
      Serial.print(F("温度EWMA基线: ")); Serial.print(tempDetector.baseline.value, 2); Serial.println(F("°C"));
      Serial.print(F("温度异常率: ")); Serial.print(tempDetector.getAnomalyRate(), 1); Serial.println(F("%"));
      Serial.print(F("湿度均值: ")); Serial.print(humiDetector.stats.getMean(), 2); Serial.println(F("%"));
      Serial.print(F("湿度标准差: ")); Serial.print(humiDetector.stats.getStdDev(), 3); Serial.println(F("%"));
      Serial.print(F("湿度异常率: ")); Serial.print(humiDetector.getAnomalyRate(), 1); Serial.println(F("%"));
      Serial.println(F("===================="));
    }
  }

  // 更新报警状态
  updateAlarm();

  // 定时采样
  if (now - lastSampleTime >= SAMPLE_INTERVAL) {
    lastSampleTime = now;

    float temp = dht.readTemperature();
    float humi = dht.readHumidity();

    if (isnan(temp) || isnan(humi)) {
      Serial.println(F("[错误] 传感器读取失败"));
      return;
    }

    sampleCount++;

    // 检测异常
    auto tempResult = tempDetector.detect(temp);
    auto humiResult = humiDetector.detect(humi);

    bool anyAnomaly = (tempResult != AnomalyDetector::NORMAL ||
                       humiResult != AnomalyDetector::NORMAL);

    // 预热阶段
    if (!warmupDone) {
      if (sampleCount >= WARMUP_SAMPLES) {
        warmupDone = true;
        Serial.println(F("✅ 预热完成，开始异常检测！"));
        Serial.print(F("基线温度: "));
        Serial.print(tempDetector.stats.getMean(), 1);
        Serial.print(F("°C ± "));
        Serial.print(tempDetector.stats.getStdDev(), 2);
        Serial.println(F("°C"));
        Serial.print(F("基线湿度: "));
        Serial.print(humiDetector.stats.getMean(), 1);
        Serial.print(F("% ± "));
        Serial.print(humiDetector.stats.getStdDev(), 2);
        Serial.println(F("%"));
        Serial.println(F("----------------------------------------"));
      } else {
        Serial.print(F("[预热 "));
        Serial.print(sampleCount);
        Serial.print(F("/"));
        Serial.print(WARMUP_SAMPLES);
        Serial.print(F("] T="));
        Serial.print(temp, 1);
        Serial.print(F("°C H="));
        Serial.print(humi, 1);
        Serial.println(F("%"));
        return;
      }
    }

    // 计算 Z-Score
    float tempZ = tempDetector.stats.getZScore(temp);
    float humiZ = humiDetector.stats.getZScore(humi);

    // 输出检测结果
    Serial.print(F("[#"));
    Serial.print(sampleCount);
    Serial.print(F("] T="));
    Serial.print(temp, 1);
    Serial.print(F("°C(Z="));
    Serial.print(tempZ, 2);
    Serial.print(F(") H="));
    Serial.print(humi, 1);
    Serial.print(F("%(Z="));
    Serial.print(humiZ, 2);
    Serial.print(F(") → "));

    if (!anyAnomaly) {
      Serial.println(F("✅ 正常"));
    } else {
      if (tempResult != AnomalyDetector::NORMAL) {
        Serial.print(F("⚠️ 温度"));
        Serial.print(getResultName(tempResult));
        Serial.print(F(" "));
        triggerAlarm("温度异常");
      }
      if (humiResult != AnomalyDetector::NORMAL) {
        Serial.print(F("⚠️ 湿度"));
        Serial.print(getResultName(humiResult));
        triggerAlarm("湿度异常");
      }
      Serial.println();
    }
  }
}