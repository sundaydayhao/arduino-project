/**
 * 光流运动检测例程
 * Arduino UNO Q (Qualcomm)
 *
 * 双脑架构：
 *   MCU (Cortex-M33) → 摄像头采集，Lucas-Kanade 光流计算，运动方向估计
 *   Linux MPU        → 接收运动事件，轨迹记录，REST API，Web 仪表盘
 *
 * 算法：稀疏光流（Lucas-Kanade），在 Cortex-M33 上实时运行
 * 输出：运动速度 (px/s)、运动方向 (角度)、运动强度
 * 应用：入侵检测、手势控制、无人机避障、人流统计
 *
 * 硬件：OV2640 摄像头（建议 QVGA 320x240 以保证帧率）
 *
 * 依赖库：Arduino_RouterBridge, camera_provider
 */

#include <Arduino_RouterBridge.h>
#include "camera_provider.h"

// ===== 图像配置 =====
constexpr int IMG_W = 160;  // 缩小分辨率提升帧率
constexpr int IMG_H = 120;

// ===== 光流配置 =====
constexpr int   OF_GRID_COLS  = 8;   // 水平采样点数
constexpr int   OF_GRID_ROWS  = 6;   // 垂直采样点数
constexpr int   OF_WIN_SIZE   = 5;   // Lucas-Kanade 窗口半径
constexpr int   OF_MAX_ITER   = 10;  // 最大迭代次数
constexpr float OF_EPSILON    = 0.01f;

// 运动检测阈值
constexpr float MOTION_THRESHOLD  = 2.0f;   // 最小运动速度 (px/帧)
constexpr float ALARM_THRESHOLD   = 15.0f;  // 告警运动速度 (px/帧)
constexpr int   SUPPRESS_MS       = 500;    // 事件抑制时间

// ===== 图像缓冲区 =====
static uint8_t prevGray[IMG_W * IMG_H];
static uint8_t currGray[IMG_W * IMG_H];
static bool    hasPrevFrame = false;

// ===== RGB 转灰度并缩放 =====
void toGray(const uint8_t* src, int srcW, int srcH,
            uint8_t* dst, int dstW, int dstH) {
  float sx = (float)srcW / dstW;
  float sy = (float)srcH / dstH;
  for (int y = 0; y < dstH; y++) {
    for (int x = 0; x < dstW; x++) {
      int px = (int)(x * sx), py = (int)(y * sy);
      int idx = (py * srcW + px) * 3;
      dst[y * dstW + x] = (uint8_t)(
        0.299f * src[idx] + 0.587f * src[idx+1] + 0.114f * src[idx+2]);
    }
  }
}

// ===== 双线性插值 =====
float bilinear(const uint8_t* img, int w, int h, float x, float y) {
  int x0 = (int)x, y0 = (int)y;
  int x1 = x0 + 1, y1 = y0 + 1;
  x0 = constrain(x0, 0, w-1); x1 = constrain(x1, 0, w-1);
  y0 = constrain(y0, 0, h-1); y1 = constrain(y1, 0, h-1);
  float dx = x - x0, dy = y - y0;
  return (1-dx)*(1-dy)*img[y0*w+x0] + dx*(1-dy)*img[y0*w+x1]
       + (1-dx)*dy*img[y1*w+x0]     + dx*dy*img[y1*w+x1];
}

// ===== Lucas-Kanade 光流（单点）=====
struct FlowVec { float vx, vy; bool valid; };

FlowVec lucasKanade(const uint8_t* prev, const uint8_t* curr,
                    int w, int h, int px, int py) {
  FlowVec result = {0, 0, false};
  float vx = 0, vy = 0;

  for (int iter = 0; iter < OF_MAX_ITER; iter++) {
    float Ixx = 0, Ixy = 0, Iyy = 0, Ixt = 0, Iyt = 0;

    for (int dy = -OF_WIN_SIZE; dy <= OF_WIN_SIZE; dy++) {
      for (int dx = -OF_WIN_SIZE; dx <= OF_WIN_SIZE; dx++) {
        float nx = px + dx + vx, ny = py + dy + vy;
        if (nx < 1 || nx >= w-1 || ny < 1 || ny >= h-1) continue;

        // 空间梯度（用前帧）
        float Ix = (bilinear(prev, w, h, nx+1, ny) -
                    bilinear(prev, w, h, nx-1, ny)) * 0.5f;
        float Iy = (bilinear(prev, w, h, nx, ny+1) -
                    bilinear(prev, w, h, nx, ny-1)) * 0.5f;
        // 时间梯度
        float It = bilinear(curr, w, h, nx, ny) -
                   bilinear(prev, w, h, px+dx, py+dy);

        Ixx += Ix * Ix;
        Ixy += Ix * Iy;
        Iyy += Iy * Iy;
        Ixt += Ix * It;
        Iyt += Iy * It;
      }
    }

    float det = Ixx * Iyy - Ixy * Ixy;
    if (fabsf(det) < 1e-6f) break;

    float dvx = -(Iyy * Ixt - Ixy * Iyt) / det;
    float dvy = -(Ixx * Iyt - Ixy * Ixt) / det;
    vx += dvx; vy += dvy;

    if (dvx*dvx + dvy*dvy < OF_EPSILON) break;
  }

  result.vx    = vx;
  result.vy    = vy;
  result.valid = (vx*vx + vy*vy) < 400.0f;  // 过滤异常大值
  return result;
}

// ===== 光流结果 =====
struct MotionResult {
  float avgVx, avgVy;    // 平均光流向量
  float speed;           // 运动速度 (px/帧)
  float direction;       // 运动方向 (度，0=右，90=下)
  float coverage;        // 运动覆盖率 (0~1)
  bool  motionDetected;
  bool  alarmTriggered;
};

MotionResult computeOpticalFlow() {
  MotionResult result = {0};

  float sumVx = 0, sumVy = 0;
  int   validCount = 0;
  int   motionCount = 0;

  int stepX = IMG_W / OF_GRID_COLS;
  int stepY = IMG_H / OF_GRID_ROWS;

  for (int gy = 0; gy < OF_GRID_ROWS; gy++) {
    for (int gx = 0; gx < OF_GRID_COLS; gx++) {
      int px = stepX / 2 + gx * stepX;
      int py = stepY / 2 + gy * stepY;

      FlowVec fv = lucasKanade(prevGray, currGray, IMG_W, IMG_H, px, py);
      if (!fv.valid) continue;

      validCount++;
      sumVx += fv.vx;
      sumVy += fv.vy;

      float mag = sqrtf(fv.vx*fv.vx + fv.vy*fv.vy);
      if (mag > MOTION_THRESHOLD) motionCount++;
    }
  }

  int totalPoints = OF_GRID_COLS * OF_GRID_ROWS;
  if (validCount == 0) return result;

  result.avgVx     = sumVx / validCount;
  result.avgVy     = sumVy / validCount;
  result.speed     = sqrtf(result.avgVx*result.avgVx + result.avgVy*result.avgVy);
  result.direction = atan2f(result.avgVy, result.avgVx) * 180.0f / M_PI;
  if (result.direction < 0) result.direction += 360.0f;
  result.coverage  = (float)motionCount / totalPoints;
  result.motionDetected = (result.speed > MOTION_THRESHOLD || result.coverage > 0.2f);
  result.alarmTriggered = (result.speed > ALARM_THRESHOLD  || result.coverage > 0.5f);

  return result;
}

// ===== 方向转文字 =====
const char* directionName(float deg) {
  if (deg < 22.5f || deg >= 337.5f) return "右";
  if (deg < 67.5f)  return "右下";
  if (deg < 112.5f) return "下";
  if (deg < 157.5f) return "左下";
  if (deg < 202.5f) return "左";
  if (deg < 247.5f) return "左上";
  if (deg < 292.5f) return "上";
  return "右上";
}

unsigned long lastEventMs  = 0;
unsigned long lastFrameMs  = 0;
int           frameCount   = 0;
int           motionEvents = 0;

void setup() {
  Bridge.begin();
  Monitor.begin();

  Monitor.println("========================================");
  Monitor.println("  Arduino UNO Q - 光流运动检测");
  Monitor.println("  MCU: Lucas-Kanade 光流 | MPU: 事件+API");
  Monitor.println("========================================");

  if (!initCamera()) {
    Monitor.println("[ERROR] 摄像头初始化失败!");
    while (1) delay(1000);
  }
  Monitor.println("[OK] 摄像头初始化成功");
  Monitor.printf("  分辨率: %dx%d | 光流网格: %dx%d\n",
                IMG_W, IMG_H, OF_GRID_COLS, OF_GRID_ROWS);
  Monitor.printf("  运动阈值: %.1f px/帧 | 告警阈值: %.1f px/帧\n\n",
                MOTION_THRESHOLD, ALARM_THRESHOLD);
  Monitor.println("开始光流运动检测...");
}

void loop() {
  uint8_t* rawImage = nullptr;
  int imgW = 0, imgH = 0;
  if (!captureImage(&rawImage, &imgW, &imgH)) { delay(10); return; }

  // 转灰度并缩放到目标分辨率
  toGray(rawImage, imgW, imgH, currGray, IMG_W, IMG_H);

  unsigned long now = millis();
  float fps = (now > lastFrameMs) ? 1000.0f / (now - lastFrameMs) : 0;
  lastFrameMs = now;
  frameCount++;

  if (!hasPrevFrame) {
    memcpy(prevGray, currGray, IMG_W * IMG_H);
    hasPrevFrame = true;
    return;
  }

  // 计算光流
  MotionResult result = computeOpticalFlow();

  // 更新前帧
  memcpy(prevGray, currGray, IMG_W * IMG_H);

  // 每 15 帧输出一次状态
  if (frameCount % 15 == 0) {
    Monitor.printf("[光流] 速度:%.1fpx/帧 方向:%s(%.0f°) 覆盖:%.0f%% FPS:%.1f\n",
                  result.speed,
                  directionName(result.direction), result.direction,
                  result.coverage * 100, fps);
  }

  // 运动事件（防抖）
  if (result.motionDetected && (now - lastEventMs > SUPPRESS_MS)) {
    lastEventMs = now;
    motionEvents++;

    if (result.alarmTriggered) {
      Monitor.printf("[!!!] 强运动告警! 速度:%.1fpx/帧 方向:%s 覆盖:%.0f%%\n",
                    result.speed, directionName(result.direction),
                    result.coverage * 100);
    } else {
      Monitor.printf("[运动] 检测到运动 速度:%.1fpx/帧 方向:%s\n",
                    result.speed, directionName(result.direction));
    }

    // ===== 上报运动事件到 Linux MPU =====
    // MPU 端 main.py 的 on_motion() 函数接收此事件
    Bridge.call("on_motion",
                result.speed,
                result.direction,
                result.coverage,
                (int)result.alarmTriggered,
                result.avgVx,
                result.avgVy,
                fps);
  }
}