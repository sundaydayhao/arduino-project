/*
 * 实验名称：在线线性回归预测
 * 实验目的：使用增量最小二乘法对温度数据进行趋势预测
 * 
 * 硬件连接：
 *   DHT11 DATA → D2
 *   DHT11 VCC  → 5V
 *   DHT11 GND  → GND
 * 
 * 算法说明：
 *   使用在线线性回归（Online Linear Regression）
 *   无需存储历史数据，每个新样本到来时增量更新模型
 *   模型：y = w * x + b（x=时间/分钟，y=温度）
 * 
 * 所需库：DHT sensor library
 */

#include <DHT.h>

#define DHT_PIN  2
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

// ============================================================
// 在线线性回归模型
// ============================================================
struct LinearRegression {
  double n;    // 样本数量
  double Sx;   // Σx
  double Sy;   // Σy
  double Sxx;  // Σ(x²)
  double Sxy;  // Σ(x*y)
  double w;    // 权重（斜率）
  double b;    // 偏置（截距）

  LinearRegression() : n(0), Sx(0), Sy(0), Sxx(0), Sxy(0), w(0), b(0) {}

  // 添加新样本并更新模型
  void addSample(double x, double y) {
    n   += 1.0;
    Sx  += x;
    Sy  += y;
    Sxx += x * x;
    Sxy += x * y;

    // 计算斜率和截距
    double denom = n * Sxx - Sx * Sx;
    if (abs(denom) > 1e-9) {
      w = (n * Sxy - Sx * Sy) / denom;
      b = (Sy - w * Sx) / n;
    }
  }

  // 预测：给定 x，返回预测的 y
  double predict(double x) {
    return w * x + b;
  }

  // 计算 R² 决定系数（需要额外统计量，此处用简化版）
  bool isValid() {
    return n >= 2;
  }

  // 获取趋势描述
  String getTrend() {
    if (n < 3) return "数据不足";
    if (w > 0.05)  return "上升趋势";
    if (w < -0.05) return "下降趋势";
    return "平稳";
  }
};

// ============================================================
// 移动平均滤波器（去除传感器噪声）
// ============================================================
struct MovingAverage {
  static const int WINDOW = 5;
  float buffer[WINDOW];
  int   index;
  int   count;

  MovingAverage() : index(0), count(0) {
    for (int i = 0; i < WINDOW; i++) buffer[i] = 0;
  }

  float update(float val) {
    buffer[index] = val;
    index = (index + 1) % WINDOW;
    if (count < WINDOW) count++;

    float sum = 0;
    for (int i = 0; i < count; i++) sum += buffer[i];
    return sum / count;
  }
};

// ============================================================
// 全局变量
// ============================================================
LinearRegression tempModel;   // 温度线性回归模型
LinearRegression humiModel;   // 湿度线性回归模型
MovingAverage    tempFilter;  // 温度滤波器
MovingAverage    humiFilter;  // 湿度滤波器

unsigned long startTime;
unsigned long lastSampleTime = 0;
const unsigned long SAMPLE_INTERVAL = 10000;  // 每10秒采样一次

int sampleCount = 0;

// ============================================================
// 打印分隔线
// ============================================================
void printSeparator() {
  Serial.println(F("----------------------------------------"));
}

void setup() {
  Serial.begin(9600);
  dht.begin();

  startTime = millis();

  Serial.println(F("========================================"));
  Serial.println(F("   Arduino 在线线性回归预测实验"));
  Serial.println(F("========================================"));
  Serial.println(F("算法：增量最小二乘法（Incremental OLS）"));
  Serial.println(F("模型：温度/湿度 = w * 时间(min) + b"));
  Serial.println(F("采样间隔：10秒"));
  printSeparator();
  Serial.println(F("等待 DHT11 稳定..."));
  delay(2000);
  Serial.println(F("开始采集数据..."));
  printSeparator();
}

void loop() {
  unsigned long now = millis();

  // 按采样间隔读取传感器
  if (now - lastSampleTime >= SAMPLE_INTERVAL) {
    lastSampleTime = now;

    float rawTemp = dht.readTemperature();
    float rawHumi = dht.readHumidity();

    if (isnan(rawTemp) || isnan(rawHumi)) {
      Serial.println(F("[错误] 传感器读取失败，跳过本次采样"));
      return;
    }

    // 滤波
    float temp = tempFilter.update(rawTemp);
    float humi = humiFilter.update(rawHumi);

    // 时间特征（分钟）
    double timeMins = (double)(now - startTime) / 60000.0;

    // 更新模型
    tempModel.addSample(timeMins, temp);
    humiModel.addSample(timeMins, humi);

    sampleCount++;

    // 打印当前读数
    Serial.print(F("[样本 #"));
    Serial.print(sampleCount);
    Serial.print(F("] 时间: "));
    Serial.print(timeMins, 2);
    Serial.print(F(" min | 温度: "));
    Serial.print(temp, 1);
    Serial.print(F("°C | 湿度: "));
    Serial.print(humi, 1);
    Serial.println(F("%"));

    // 模型有效后输出预测结果
    if (tempModel.isValid()) {
      // 预测未来 5 分钟和 10 分钟的温度
      double pred5  = tempModel.predict(timeMins + 5.0);
      double pred10 = tempModel.predict(timeMins + 10.0);

      Serial.print(F("  📈 温度模型: y = "));
      Serial.print(tempModel.w, 4);
      Serial.print(F(" * x + "));
      Serial.println(tempModel.b, 2);

      Serial.print(F("  🔮 温度预测: +5min="));
      Serial.print(pred5, 1);
      Serial.print(F("°C  +10min="));
      Serial.print(pred10, 1);
      Serial.println(F("°C"));

      Serial.print(F("  📊 温度趋势: "));
      Serial.println(tempModel.getTrend());

      // 湿度预测
      double humiPred5 = humiModel.predict(timeMins + 5.0);
      Serial.print(F("  💧 湿度预测: +5min="));
      Serial.print(humiPred5, 1);
      Serial.print(F("%  趋势: "));
      Serial.println(humiModel.getTrend());
    } else {
      Serial.println(F("  ⏳ 正在收集数据，需要至少2个样本建立模型..."));
    }

    printSeparator();
  }

  // 每分钟输出一次模型摘要（样本数 >= 5 后）
  static unsigned long lastSummary = 0;
  if (sampleCount >= 5 && now - lastSummary >= 60000) {
    lastSummary = now;
    double currentTime = (double)(now - startTime) / 60000.0;

    Serial.println(F("===== 模型摘要 ====="));
    Serial.print(F("已收集样本: "));
    Serial.println(sampleCount);
    Serial.print(F("温度模型斜率(w): "));
    Serial.print(tempModel.w, 4);
    Serial.println(F(" °C/min"));
    Serial.print(F("温度模型截距(b): "));
    Serial.print(tempModel.b, 2);
    Serial.println(F(" °C"));
    Serial.print(F("当前温度预测: "));
    Serial.print(tempModel.predict(currentTime), 1);
    Serial.println(F(" °C"));
    Serial.print(F("30分钟后预测: "));
    Serial.print(tempModel.predict(currentTime + 30), 1);
    Serial.println(F(" °C"));
    printSeparator();
  }
}