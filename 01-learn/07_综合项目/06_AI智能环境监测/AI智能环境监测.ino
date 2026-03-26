/*
 * 综合项目：AI 智能环境监测系统
 * 
 * 功能：
 *   - KNN 环境状态分类（舒适/炎热/潮湿/干燥/昏暗）
 *   - 在线线性回归温度趋势预测
 *   - Z-Score + EWMA 异常检测
 *   - OLED 实时显示
 *   - 串口 CSV 数据输出
 * 
 * 硬件连接：
 *   DHT11 DATA   → D2
 *   光敏电阻分压 → A0
 *   OLED SDA     → A4
 *   OLED SCL     → A5
 *   蜂鸣器       → D8
 *   红色 LED     → D12（报警）
 *   绿色 LED     → D13（正常）
 * 
 * 所需库：
 *   DHT sensor library
 *   Adafruit SSD1306
 *   Adafruit GFX Library
 */

#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ============================================================
// 硬件配置
// ============================================================
#define DHT_PIN    2
#define DHT_TYPE   DHT11
#define LDR_PIN    A0
#define BUZZER_PIN 8
#define LED_RED    12
#define LED_GREEN  13

#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_ADDR   0x3C

DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// ============================================================
// ① 在线线性回归
// ============================================================
struct LinearRegression {
  double n, Sx, Sy, Sxx, Sxy, w, b;
  LinearRegression() : n(0), Sx(0), Sy(0), Sxx(0), Sxy(0), w(0), b(0) {}

  void addSample(double x, double y) {
    n++; Sx += x; Sy += y; Sxx += x*x; Sxy += x*y;
    double d = n*Sxx - Sx*Sx;
    if (abs(d) > 1e-9) { w = (n*Sxy - Sx*Sy)/d; b = (Sy - w*Sx)/n; }
  }
  double predict(double x) { return w*x + b; }
  bool   isValid()          { return n >= 3; }
};

// ============================================================
// ② 移动窗口统计（用于异常检测）
// ============================================================
#define WIN_SIZE 15

struct WindowStats {
  float  buf[WIN_SIZE];
  int    head, cnt;
  double sum, sumSq;

  WindowStats() : head(0), cnt(0), sum(0), sumSq(0) {
    for (int i = 0; i < WIN_SIZE; i++) buf[i] = 0;
  }

  void push(float v) {
    if (cnt == WIN_SIZE) { float o=buf[head]; sum-=o; sumSq-=(double)o*o; }
    else cnt++;
    buf[head]=v; head=(head+1)%WIN_SIZE;
    sum+=v; sumSq+=(double)v*v;
  }

  float mean()   { return cnt ? (float)(sum/cnt) : 0; }
  float stddev() {
    if (cnt < 2) return 0;
    double m=sum/cnt, var=sumSq/cnt - m*m;
    return (float)sqrt(var < 0 ? 0 : var);
  }
  float zscore(float v) {
    float s=stddev();
    return (s < 0.01f) ? 0 : (v - mean())/s;
  }
  bool ready() { return cnt >= WIN_SIZE/2; }
};

// ============================================================
// ③ KNN 分类器（精简版，5类）
// ============================================================
#define KNN_FEATURES 3
#define KNN_CLASSES  5
#define KNN_SAMPLES  25
#define KNN_K        3

const char* KNN_LABELS[] = {"Comfort","Hot","Humid","Dry","Dark"};
const char* KNN_LABELS_CN[] = {"舒适","炎热","潮湿","干燥","昏暗"};

// 归一化范围
const float FEAT_MIN[KNN_FEATURES] = {0.0f,   0.0f,   0.0f};
const float FEAT_MAX[KNN_FEATURES] = {50.0f, 100.0f, 1023.0f};

// 预置训练数据 [temp, humi, ldr]
float knnData[KNN_SAMPLES][KNN_FEATURES] = {
  {22,50,600},{24,45,700},{21,55,500},{23,48,650},{25,52,580},  // 舒适
  {32,55,800},{35,40,900},{30,60,750},{33,45,820},{38,35,950},  // 炎热
  {25,80,400},{22,85,350},{28,75,500},{24,90,300},{26,78,450},  // 潮湿
  {24,20,700},{26,15,800},{22,25,600},{28,18,750},{25,22,680},  // 干燥
  {22,50,80}, {24,55,100},{20,45,60}, {23,52,120},{25,48,90},  // 昏暗
};
int knnLabels[KNN_SAMPLES] = {
  0,0,0,0,0, 1,1,1,1,1, 2,2,2,2,2, 3,3,3,3,3, 4,4,4,4,4
};

float knnNorm(float v, int fi) {
  float r = FEAT_MAX[fi] - FEAT_MIN[fi];
  return r < 1e-6f ? 0 : (v - FEAT_MIN[fi]) / r;
}

int knnPredict(float* feat, float* conf) {
  float dists[KNN_SAMPLES];
  for (int i = 0; i < KNN_SAMPLES; i++) {
    float s = 0;
    for (int j = 0; j < KNN_FEATURES; j++) {
      float d = knnNorm(feat[j],j) - knnNorm(knnData[i][j],j);
      s += d*d;
    }
    dists[i] = sqrt(s);
  }
  // 找 K 近邻
  int   ki[KNN_K]; float kd[KNN_K];
  for (int k=0;k<KNN_K;k++) kd[k]=1e9f;
  for (int i=0;i<KNN_SAMPLES;i++) {
    int mx=0;
    for (int k=1;k<KNN_K;k++) if(kd[k]>kd[mx]) mx=k;
    if (dists[i]<kd[mx]) { kd[mx]=dists[i]; ki[mx]=i; }
  }
  int votes[KNN_CLASSES]={0};
  for (int k=0;k<KNN_K;k++) votes[knnLabels[ki[k]]]++;
  int best=0;
  for (int c=1;c<KNN_CLASSES;c++) if(votes[c]>votes[best]) best=c;
  if (conf) *conf = (float)votes[best]/KNN_K;
  return best;
}

// ============================================================
// 全局状态
// ============================================================
LinearRegression tempModel;
WindowStats      tempStats, humiStats;

unsigned long startTime;
unsigned long lastSample5s  = 0;
unsigned long lastSample10s = 0;
int           sampleCount   = 0;
bool          warmupDone    = false;
const int     WARMUP_N      = 10;

// 当前读数
float curTemp = 0, curHumi = 0, curLDR = 0;
int   curClass = 0;
float curConf  = 0;
float tempPred10 = 0;
bool  isAnomaly  = false;

// 报警
bool  alarmActive = false;
unsigned long alarmEnd = 0;

void triggerAlarm() {
  alarmActive = true;
  alarmEnd = millis() + 3000;
  digitalWrite(LED_RED,   HIGH);
  digitalWrite(LED_GREEN, LOW);
}

void updateAlarm() {
  if (alarmActive && millis() > alarmEnd) {
    alarmActive = false;
    digitalWrite(LED_RED,   LOW);
    digitalWrite(LED_GREEN, HIGH);
    noTone(BUZZER_PIN);
  }
  if (alarmActive) {
    static unsigned long lb = 0;
    static bool bs = false;
    if (millis()-lb > 400) { lb=millis(); bs=!bs; if(bs) tone(BUZZER_PIN,2200,350); }
  }
}

// ============================================================
// OLED 显示（4个页面轮换）
// ============================================================
int displayPage = 0;
unsigned long lastPageSwitch = 0;

void updateOLED() {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);

  switch (displayPage) {
    case 0:  // 传感器数据
      oled.setTextSize(1);
      oled.setCursor(0, 0);
      oled.println(F("=== Sensor Data ==="));
      oled.print(F("Temp: ")); oled.print(curTemp,1); oled.println(F(" C"));
      oled.print(F("Humi: ")); oled.print(curHumi,1); oled.println(F(" %"));
      oled.print(F("LDR:  ")); oled.println((int)curLDR);
      oled.print(F("Samples: ")); oled.println(sampleCount);
      break;

    case 1:  // AI 分类结果
      oled.setTextSize(1);
      oled.setCursor(0, 0);
      oled.println(F("=== AI Status ==="));
      oled.setTextSize(2);
      oled.setCursor(0, 16);
      oled.println(KNN_LABELS[curClass]);
      oled.setTextSize(1);
      oled.print(F("Conf: ")); oled.print((int)(curConf*100)); oled.println(F("%"));
      oled.print(isAnomaly ? F("!! ANOMALY !!") : F("Normal"));
      break;

    case 2:  // 趋势预测
      oled.setTextSize(1);
      oled.setCursor(0, 0);
      oled.println(F("=== Prediction ==="));
      oled.print(F("Now:  ")); oled.print(curTemp,1); oled.println(F(" C"));
      if (tempModel.isValid()) {
        oled.print(F("+10m: ")); oled.print(tempPred10,1); oled.println(F(" C"));
        float delta = tempPred10 - curTemp;
        oled.print(F("Trend: "));
        if (delta > 0.5f)       oled.println(F("Rising"));
        else if (delta < -0.5f) oled.println(F("Falling"));
        else                    oled.println(F("Stable"));
        oled.print(F("Slope: ")); oled.print(tempModel.w,3); oled.println(F("/min"));
      } else {
        oled.println(F("Collecting..."));
      }
      break;

    case 3:  // 统计信息
      oled.setTextSize(1);
      oled.setCursor(0, 0);
      oled.println(F("=== Statistics ==="));
      oled.print(F("T mean: ")); oled.print(tempStats.mean(),1); oled.println(F("C"));
      oled.print(F("T std:  ")); oled.print(tempStats.stddev(),2); oled.println(F("C"));
      oled.print(F("H mean: ")); oled.print(humiStats.mean(),1); oled.println(F("%"));
      oled.print(F("Uptime: ")); oled.print((millis()-startTime)/60000); oled.println(F("min"));
      break;
  }

  oled.display();
}

// ============================================================
// 串口 CSV 输出
// ============================================================
void printCSV() {
  float tempZ = tempStats.zscore(curTemp);
  Serial.print(millis()); Serial.print(F(","));
  Serial.print(curTemp,2); Serial.print(F(","));
  Serial.print(curHumi,2); Serial.print(F(","));
  Serial.print((int)curLDR); Serial.print(F(","));
  Serial.print(KNN_LABELS[curClass]); Serial.print(F(","));
  Serial.print(tempZ,3); Serial.print(F(","));
  Serial.print(tempModel.isValid() ? tempPred10 : curTemp, 2); Serial.print(F(","));
  Serial.println(isAnomaly ? 1 : 0);
}

void setup() {
  Serial.begin(9600);
  dht.begin();

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_RED,    OUTPUT);
  pinMode(LED_GREEN,  OUTPUT);
  digitalWrite(LED_GREEN, HIGH);

  // 初始化 OLED
  bool oledOK = oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  if (oledOK) {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(0, 0);
    oled.println(F("AI Environment"));
    oled.println(F("Monitor v1.0"));
    oled.println();
    oled.println(F("Algorithms:"));
    oled.println(F("- KNN Classifier"));
    oled.println(F("- Linear Regression"));
    oled.println(F("- Anomaly Detection"));
    oled.display();
  }

  startTime = millis();

  Serial.println(F("========================================"));
  Serial.println(F("   AI 智能环境监测系统"));
  Serial.println(F("========================================"));
  Serial.println(F("算法: KNN + 线性回归 + 异常检测"));
  Serial.println(F("CSV格式: 时间,温度,湿度,光照,状态,Z分数,预测温度,异常"));
  Serial.println(F("----------------------------------------"));
  Serial.println(F("time_ms,temp,humi,ldr,status,temp_z,pred_temp,anomaly"));

  delay(2000);
  Serial.println(F("开始预热..."));
}

void loop() {
  unsigned long now = millis();

  updateAlarm();

  // 切换 OLED 页面（每4秒）
  if (now - lastPageSwitch >= 4000) {
    lastPageSwitch = now;
    displayPage = (displayPage + 1) % 4;
    updateOLED();
  }

  // 每5秒：读取传感器 + KNN分类 + 异常检测
  if (now - lastSample5s >= 5000) {
    lastSample5s = now;

    float t = dht.readTemperature();
    float h = dht.readHumidity();
    float l = analogRead(LDR_PIN);

    if (isnan(t) || isnan(h)) {
      Serial.println(F("[错误] 传感器读取失败"));
      return;
    }

    curTemp = t; curHumi = h; curLDR = l;
    sampleCount++;

    // 更新统计
    tempStats.push(t);
    humiStats.push(h);

    // 异常检测
    float tz = tempStats.zscore(t);
    float hz = humiStats.zscore(h);
    isAnomaly = tempStats.ready() && (abs(tz) > 2.5f || abs(hz) > 2.5f);
    if (isAnomaly) triggerAlarm();

    // KNN 分类
    float feat[KNN_FEATURES] = {t, h, l};
    curClass = knnPredict(feat, &curConf);

    // 预热检查
    if (!warmupDone && sampleCount >= WARMUP_N) {
      warmupDone = true;
      Serial.println(F("✅ 预热完成，AI 分析已启动"));
      Serial.print(F("基线温度: ")); Serial.print(tempStats.mean(),1);
      Serial.print(F("°C ± ")); Serial.print(tempStats.stddev(),2); Serial.println(F("°C"));
    }

    // 输出 CSV
    if (warmupDone) printCSV();

    // 更新 OLED
    updateOLED();
  }

  // 每10秒：更新线性回归
  if (now - lastSample10s >= 10000) {
    lastSample10s = now;
    double timeMins = (double)(now - startTime) / 60000.0;
    tempModel.addSample(timeMins, curTemp);
    if (tempModel.isValid()) {
      tempPred10 = (float)tempModel.predict(timeMins + 10.0);
    }

    // 详细串口输出
    if (warmupDone) {
      Serial.print(F("[AI] 状态: "));
      Serial.print(KNN_LABELS_CN[curClass]);
      Serial.print(F("("));
      Serial.print((int)(curConf*100));
      Serial.print(F("%)"));
      if (tempModel.isValid()) {
        Serial.print(F("  趋势: "));
        float delta = tempPred10 - curTemp;
        if (delta > 0.5f)       Serial.print(F("↑上升"));
        else if (delta < -0.5f) Serial.print(F("↓下降"));
        else                    Serial.print(F("→平稳"));
        Serial.print(F("  +10min预测: "));
        Serial.print(tempPred10, 1);
        Serial.print(F("°C"));
      }
      if (isAnomaly) Serial.print(F("  ⚠️ 异常！"));
      Serial.println();
    }
  }
}