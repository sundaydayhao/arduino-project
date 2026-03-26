/**
 * ChatGPT API 对话助手 - MCU 端
 * Arduino UNO Q (Qualcomm)
 *
 * 双脑架构：
 *   MCU (Cortex-M33) → 按键触发，OLED 显示，发送问题，接收回复
 *   Linux MPU        → 调用 OpenAI API，管理对话历史，返回回复
 *
 * 交互方式：
 *   - 按 BUTTON_PIN 触发预设问题（演示模式）
 *   - 或通过串口输入问题
 *   - 回复显示在 OLED 并通过串口输出
 *
 * 依赖库：Adafruit_SSD1306, Arduino_RouterBridge
 */

#include <Arduino_RouterBridge.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

// ===== 硬件配置 =====
#define BUTTON_PIN   2
#define OLED_WIDTH   128
#define OLED_HEIGHT  64
#define OLED_ADDR    0x3C

Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// ===== 预设演示问题 =====
const char* DEMO_QUESTIONS[] = {
  "Arduino UNO Q 有哪些特色功能?",
  "如何用 TFLite 在 MCU 上运行 AI?",
  "Matter 协议是什么?",
  "Qualcomm QCS6490 的 AI 算力是多少?",
  "如何优化嵌入式 AI 模型的推理速度?",
};
constexpr int DEMO_COUNT = sizeof(DEMO_QUESTIONS) / sizeof(DEMO_QUESTIONS[0]);
int demoIdx = 0;

// ===== OLED 显示多行文字 =====
void oledShowText(const char* title, const char* body) {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);

  // 标题行
  oled.setCursor(0, 0);
  oled.println(title);
  oled.drawLine(0, 10, OLED_WIDTH, 10, SSD1306_WHITE);

  // 正文（自动换行）
  oled.setCursor(0, 14);
  oled.setTextWrap(true);
  oled.print(body);

  oled.display();
}

// ===== 滚动显示长文本 =====
void oledScrollText(const char* text, int delayMs = 80) {
  int len = strlen(text);
  for (int i = 0; i < len - 20 && i < 200; i++) {
    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextWrap(true);
    oled.print(text + i);
    oled.display();
    delay(delayMs);
  }
}

void setup() {
  Bridge.begin();
  Monitor.begin();

  Monitor.println("========================================");
  Monitor.println("  Arduino UNO Q - ChatGPT 对话助手");
  Monitor.println("  MCU: 交互控制 | MPU: OpenAI API");
  Monitor.println("========================================");

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // 初始化 OLED
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Monitor.println("[WARN] OLED 初始化失败，仅串口输出");
  } else {
    oledShowText("Arduino UNO Q", "ChatGPT 助手\n按键提问...");
    Monitor.println("[OK] OLED 初始化成功");
  }

  Monitor.println("[OK] 系统就绪，按 D2 按键提问\n");
}

void loop() {
  // ===== 按键触发 =====
  static bool lastBtn = HIGH;
  bool curBtn = digitalRead(BUTTON_PIN);

  if (lastBtn == HIGH && curBtn == LOW) {
    // 按键按下
    const char* question = DEMO_QUESTIONS[demoIdx % DEMO_COUNT];
    demoIdx++;

    Monitor.printf("\n[问] %s\n", question);
    oledShowText("提问中...", question);

    // ===== 调用 Linux MPU 的 chat() 函数 =====
    // MPU 端 main.py 的 chat() 函数处理 OpenAI API 调用并返回回复
    String answer = Bridge.call("chat", question);

    Monitor.printf("[答] %s\n\n", answer.c_str());
    oledShowText("回复:", answer.c_str());

    // 长回复滚动显示
    if (answer.length() > 80) {
      delay(2000);
      oledScrollText(answer.c_str());
    }

    delay(500);  // 防抖
  }
  lastBtn = curBtn;

  // ===== 串口输入问题 =====
  if (Monitor.available()) {
    String question = Monitor.readStringUntil('\n');
    question.trim();
    if (question.length() > 0) {
      Monitor.printf("\n[问] %s\n", question.c_str());
      oledShowText("提问中...", question.c_str());

      String answer = Bridge.call("chat", question.c_str());

      Monitor.printf("[答] %s\n\n", answer.c_str());
      oledShowText("回复:", answer.c_str());
    }
  }
}