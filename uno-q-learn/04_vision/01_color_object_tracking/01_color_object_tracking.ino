/**
 * 颜色目标追踪 - MCU 端
 * Arduino UNO Q (Qualcomm)
 *
 * 双脑架构：
 *   MCU (Cortex-M33) → 摄像头采集，HSV 颜色检测，PID 云台控制
 *   Linux MPU        → 接收追踪数据，记录轨迹，REST API，Web 仪表盘
 *
 * 算法：HSV 颜色空间阈值分割 + 连通域质心计算 + PID 控制
 * 硬件：OV2640 摄像头 + 2 轴舵机云台
 *
 * 依赖库：Arduino_RouterBridge, Servo
 */

#include <Arduino_RouterBridge.h>
#include <Servo.h>
#include "camera_provider.h"

// ===== 图像配置 =====
constexpr int IMG_W = 320;
constexpr int IMG_H = 240;

// ===== 追踪颜色 HSV 阈值（红色）=====
// 可修改为其他颜色
struct HSVRange {
  int hMin, hMax;
  int sMin, sMax;
  int vMin, vMax;
  const char* name;
};

HSVRange TARGET_COLORS[] = {
  {0, 10,  120, 255, 80, 255, "red"},
  {160, 180, 120, 255, 80, 255, "red2"},  // 红色跨越 180
  {35, 85,  80, 255, 80, 255, "green"},
  {100, 130, 80, 255, 80, 255, "blue"},
};
int activeColor = 0;  // 0=红色, 2=绿色, 3=蓝色

// ===== 舵机配置 =====
#define PAN_PIN   9
#define TILT_PIN  10
Servo panServo, tiltServo;

float panAngle  = 90.0f;
float tiltAngle = 90.0f;

// ===== PID 控制器 =====
struct PID {
  float kp, ki, kd;
  float integral, prevError;
};

PID panPID  = {0.15f, 0.001f, 0.05f, 0, 0};
PID tiltPID = {0.15f, 0.001f, 0.05f, 0, 0};

float pidUpdate(PID& pid, float error, float dt) {
  pid.integral  += error * dt;
  pid.integral   = constrain(pid.integral, -50, 50);
  float deriv    = (error - pid.prevError) / dt;
  pid.prevError  = error;
  return pid.kp * error + pid.ki * pid.integral + pid.kd * deriv;
}

// ===== RGB 转 HSV =====
void rgbToHSV(uint8_t r, uint8_t g, uint8_t b,
              float& h, float& s, float& v) {
  float rf = r / 255.0f, gf = g / 255.0f, bf = b / 255.0f;
  float cmax = max(rf, max(gf, bf));
  float cmin = min(rf, min(gf, bf));
  float diff = cmax - cmin;

  v = cmax;
  s = (cmax == 0) ? 0 : diff / cmax;

  if (diff == 0) { h = 0; return; }
  if (cmax == rf) h = 60.0f * fmod((gf - bf) / diff, 6.0f);
  else if (cmax == gf) h = 60.0f * ((bf - rf) / diff + 2.0f);
  else h = 60.0f * ((rf - gf) / diff + 4.0f);
  if (h < 0) h += 360.0f;
}

// ===== 颜色检测，返回质心 =====
bool detectColor(const uint8_t* img, int w, int h,
                 int& cx, int& cy, int& area) {
  long sumX = 0, sumY = 0;
  area = 0;

  for (int y = 0; y < h; y += 2) {  // 步进 2 加速
    for (int x = 0; x < w; x += 2) {
      int idx = (y * w + x) * 3;
      float hv, sv, vv;
      rgbToHSV(img[idx], img[idx+1], img[idx+2], hv, sv, vv);

      int hi = (int)hv, si = (int)(sv * 255), vi = (int)(vv * 255);
      bool match = false;
      for (int c = activeColor; c <= activeColor + 1 && c < 4; c++) {
        auto& t = TARGET_COLORS[c];
        if (hi >= t.hMin && hi <= t.hMax &&
            si >= t.sMin && si <= t.sMax &&
            vi >= t.vMin && vi <= t.vMax) {
          match = true; break;
        }
      }
      if (match) { sumX += x; sumY += y; area++; }
    }
  }

  if (area < 50) return false;  // 最小面积过滤
  cx = (int)(sumX / area);
  cy = (int)(sumY / area);
  return true;
}

unsigned long lastFrameMs = 0;
int frameCount = 0;

void setup() {
  Bridge.begin();
  Monitor.begin();

  Monitor.println("========================================");
  Monitor.println("  Arduino UNO Q - 颜色目标追踪");
  Monitor.println("  MCU: 视觉+PID控制 | MPU: 轨迹+API");
  Monitor.println("========================================");

  panServo.attach(PAN_PIN);
  tiltServo.attach(TILT_PIN);
  panServo.write(90);
  tiltServo.write(90);
  Monitor.println("[OK] 舵机初始化成功，归中");

  if (!initCamera()) {
    Monitor.println("[ERROR] 摄像头初始化失败!");
    while (1) delay(1000);
  }
  Monitor.println("[OK] 摄像头初始化成功");
  Monitor.printf("[OK] 追踪颜色: %s\n\n", TARGET_COLORS[activeColor].name);
}

void loop() {
  uint8_t* img = nullptr;
  int imgW = 0, imgH = 0;
  if (!captureImage(&img, &imgW, &imgH)) { delay(10); return; }

  unsigned long now = millis();
  float dt = (now - lastFrameMs) / 1000.0f;
  if (dt <= 0) dt = 0.033f;
  lastFrameMs = now;
  frameCount++;

  int cx = 0, cy = 0, area = 0;
  bool detected = detectColor(img, imgW, imgH, cx, cy, area);

  if (detected) {
    // PID 控制云台
    float errX = cx - imgW / 2.0f;
    float errY = cy - imgH / 2.0f;
    panAngle  -= pidUpdate(panPID,  errX, dt);
    tiltAngle += pidUpdate(tiltPID, errY, dt);
    panAngle  = constrain(panAngle,  10, 170);
    tiltAngle = constrain(tiltAngle, 10, 170);
    panServo.write((int)panAngle);
    tiltServo.write((int)tiltAngle);
  }

  float fps = 1.0f / dt;

  // 每 10 帧输出一次
  if (frameCount % 10 == 0) {
    if (detected) {
      Monitor.printf("[追踪] 中心:(%d,%d) 面积:%d 云台:(%.0f,%.0f) FPS:%.1f\n",
                    cx, cy, area, panAngle, tiltAngle, fps);
    } else {
      Monitor.printf("[搜索] 云台:(%.0f,%.0f) FPS:%.1f\n",
                    panAngle, tiltAngle, fps);
    }
  }

  // ===== 上报追踪数据到 Linux MPU =====
  Bridge.call("update_tracking",
              (int)detected,
              TARGET_COLORS[activeColor].name,
              cx, cy, area,
              panAngle, tiltAngle, fps);
}