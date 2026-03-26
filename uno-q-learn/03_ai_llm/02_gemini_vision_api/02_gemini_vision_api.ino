/**
 * Gemini Vision API 图像理解 - MCU 端
 * Arduino UNO Q (Qualcomm)
 *
 * 双脑架构：
 *   MCU (Cortex-M33) → 摄像头拍照，JPEG 压缩，Base64 编码，发送给 MPU
 *   Linux MPU        → 调用 Gemini Vision API，返回图像描述
 *
 * 流程：
 *   1. MCU 拍照 → JPEG 压缩 → Base64 编码
 *   2. Bridge.call("analyze_image", b64, prompt) → MPU 调用 Gemini API
 *   3. MPU 返回描述文字 → MCU 显示在 OLED
 *
 * 依赖库：Adafruit_SSD1306, Arduino_RouterBridge, camera_provider
 */

#include <Arduino_RouterBridge.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "camera_provider.h"
#include "jpeg_encoder.h"   // JPEG 压缩（Qualcomm 硬件加速）
#include "base64_utils.h"   // Base64 编码工具

// ===== 硬件配置 =====
#define BUTTON_PIN  2
#define OLED_ADDR   0x3C

Adafruit_SSD1306 oled(128, 64, &Wire, -1);

// ===== 预设提示词 =====
const char* PROMPTS[] = {
  "请用中文描述这张图片的内容",
  "图片中有哪些物体？",
  "这张图片的场景是什么？",
  "图片中有人吗？如果有，他们在做什么？",
};
constexpr int PROMPT_COUNT = sizeof(PROMPTS) / sizeof(PROMPTS[0]);
int promptIdx = 0;

// ===== OLED 显示 =====
void oledShow(const char* title, const char* body) {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.println(title);
  oled.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  oled.setCursor(0, 14);
  oled.setTextWrap(true);
  oled.print(body);
  oled.display();
}

void setup() {
  Bridge.begin();
  Monitor.begin();

  Monitor.println("========================================");
  Monitor.println("  Arduino UNO Q - Gemini Vision API");
  Monitor.println("  MCU: 摄像头+编码 | MPU: Gemini API");
  Monitor.println("========================================");

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (!initCamera()) {
    Monitor.println("[ERROR] 摄像头初始化失败!");
    while (1) delay(1000);
  }
  Monitor.println("[OK] 摄像头初始化成功");

  if (oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    oledShow("Gemini Vision", "按键拍照分析...");
    Monitor.println("[OK] OLED 初始化成功");
  }

  Monitor.println("[OK] 系统就绪，按 D2 拍照\n");
}

void loop() {
  static bool lastBtn = HIGH;
  bool curBtn = digitalRead(BUTTON_PIN);

  if (lastBtn == HIGH && curBtn == LOW) {
    const char* prompt = PROMPTS[promptIdx % PROMPT_COUNT];
    promptIdx++;

    Monitor.println("\n[拍照] 正在拍照...");
    oledShow("拍照中...", prompt);

    // 拍照
    uint8_t* rawImage = nullptr;
    int imgW = 0, imgH = 0;
    if (!captureImage(&rawImage, &imgW, &imgH)) {
      Monitor.println("[ERROR] 拍照失败");
      oledShow("错误", "拍照失败");
      delay(500);
      lastBtn = curBtn;
      return;
    }
    Monitor.printf("[OK] 拍照成功 %dx%d\n", imgW, imgH);

    // JPEG 压缩（降低传输数据量）
    static uint8_t jpegBuf[32 * 1024];  // 32KB JPEG 缓冲
    int jpegLen = encodeJPEG(rawImage, imgW, imgH, jpegBuf, sizeof(jpegBuf), 70);
    Monitor.printf("[OK] JPEG 压缩: %d bytes\n", jpegLen);

    // Base64 编码
    static char b64Buf[48 * 1024];  // Base64 约为原始的 4/3
    int b64Len = base64Encode(jpegBuf, jpegLen, b64Buf, sizeof(b64Buf));
    Monitor.printf("[OK] Base64 编码: %d bytes\n", b64Len);

    oledShow("分析中...", "正在调用 Gemini...");
    Monitor.printf("[API] 提示: %s\n", prompt);

    // ===== 调用 Linux MPU 的 analyze_image() 函数 =====
    // MPU 端 main.py 的 analyze_image() 调用 Gemini Vision API
    String description = Bridge.call("analyze_image", b64Buf, prompt);

    Monitor.printf("[结果] %s\n\n", description.c_str());
    oledShow("Gemini 说:", description.c_str());

    delay(500);
  }
  lastBtn = curBtn;
}