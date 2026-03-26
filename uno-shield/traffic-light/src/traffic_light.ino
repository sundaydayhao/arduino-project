/*
 * Arduino 交通灯控制器（基础版）
 * ================================
 * 功能：
 *   - 标准交通灯时序：红灯 → 绿灯 → 黄灯（循环）
 *   - 行人过街按钮（外部中断，Pin 2）
 *   - 蜂鸣器行人提示音
 *   - 串口实时状态输出
 *   - 绿灯末尾闪烁提示
 *
 * 硬件连接：
 *   红灯 LED  -> Pin 13 (串联 220Ω 电阻到 GND)
 *   黄灯 LED  -> Pin 12 (串联 220Ω 电阻到 GND)
 *   绿灯 LED  -> Pin 11 (串联 220Ω 电阻到 GND)
 *   行人按钮  -> Pin 2  (另一端接 GND，使用内部上拉)
 *   蜂鸣器    -> Pin 8  (无源蜂鸣器，另一端接 GND)
 *
 * 时序配置（可修改）：
 *   红灯：5秒
 *   绿灯：4秒（最后1秒闪烁）
 *   黄灯：2秒（闪烁）
 *   行人过街：红灯延长至8秒
 */

// ─── 引脚定义 ────────────────────────────────────────────────
#define PIN_RED     13
#define PIN_YELLOW  12
#define PIN_GREEN   11
#define PIN_BUTTON   2   // 外部中断 INT0
#define PIN_BUZZER   8

// ─── 时序配置（毫秒）────────────────────────────────────────
#define TIME_RED          5000UL   // 正常红灯时长
#define TIME_GREEN        4000UL   // 绿灯时长
#define TIME_YELLOW       2000UL   // 黄灯时长
#define TIME_PEDESTRIAN   8000UL   // 行人过街红灯时长
#define BLINK_INTERVAL     400UL   // 闪烁间隔

// ─── 状态机 ──────────────────────────────────────────────────
enum TrafficState : uint8_t {
  STATE_RED    = 0,
  STATE_GREEN  = 1,
  STATE_YELLOW = 2
};

TrafficState currentState    = STATE_RED;
unsigned long stateStartTime = 0;
volatile bool pedestrianReq  = false;   // 中断设置
bool pedestrianActive        = false;   // 当前红灯是否为行人模式

// ─── 中断服务程序 ─────────────────────────────────────────────
void ISR_pedestrianButton() {
  pedestrianReq = true;
}

// ─── 灯光控制 ─────────────────────────────────────────────────
void setLights(bool red, bool yellow, bool green) {
  digitalWrite(PIN_RED,    red    ? HIGH : LOW);
  digitalWrite(PIN_YELLOW, yellow ? HIGH : LOW);
  digitalWrite(PIN_GREEN,  green  ? HIGH : LOW);
}

// ─── 蜂鸣器 ──────────────────────────────────────────────────
void beepOnce(int freq, int durationMs) {
  tone(PIN_BUZZER, freq, durationMs);
}

// ─── 串口状态打印 ─────────────────────────────────────────────
void printState(const char* stateName, unsigned long durationMs) {
  Serial.print("[");
  Serial.print(millis() / 1000);
  Serial.print("s] → ");
  Serial.print(stateName);
  Serial.print(" (");
  Serial.print(durationMs / 1000);
  Serial.println("s)");
}

// ─── Setup ────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);

  pinMode(PIN_RED,    OUTPUT);
  pinMode(PIN_YELLOW, OUTPUT);
  pinMode(PIN_GREEN,  OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  // 行人按钮：下降沿触发（按下时 HIGH→LOW）
  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON),
                  ISR_pedestrianButton, FALLING);

  // 初始状态：红灯
  setLights(true, false, false);
  stateStartTime = millis();

  Serial.println("=== Traffic Light Controller ===");
  Serial.println("Pin 13=RED  Pin 12=YELLOW  Pin 11=GREEN");
  Serial.println("Pin 2=Pedestrian Button  Pin 8=Buzzer");
  Serial.println("--------------------------------");
  printState("RED", TIME_RED);
}

// ─── Loop ─────────────────────────────────────────────────────
void loop() {
  unsigned long now     = millis();
  unsigned long elapsed = now - stateStartTime;

  switch (currentState) {

    // ── 红灯 ──────────────────────────────────────────────────
    case STATE_RED: {
      unsigned long redDuration = pedestrianActive ? TIME_PEDESTRIAN : TIME_RED;

      // 行人过街期间：每秒短鸣一次
      if (pedestrianActive) {
        static unsigned long lastBeep = 0;
        if (now - lastBeep >= 1000) {
          beepOnce(880, 80);
          lastBeep = now;
        }
      }

      if (elapsed >= redDuration) {
        pedestrianActive = false;
        currentState     = STATE_GREEN;
        setLights(false, false, true);
        stateStartTime   = millis();
        beepOnce(1200, 150);
        printState("GREEN", TIME_GREEN);
      }
      break;
    }

    // ── 绿灯 ──────────────────────────────────────────────────
    case STATE_GREEN: {
      // 最后 1 秒：绿灯闪烁
      if (elapsed >= TIME_GREEN - 1000) {
        bool on = ((now / BLINK_INTERVAL) % 2 == 0);
        setLights(false, false, on);
      }

      if (elapsed >= TIME_GREEN) {
        currentState   = STATE_YELLOW;
        setLights(false, true, false);
        stateStartTime = millis();
        beepOnce(800, 100);
        printState("YELLOW", TIME_YELLOW);
      }
      break;
    }

    // ── 黄灯 ──────────────────────────────────────────────────
    case STATE_YELLOW: {
      // 黄灯全程闪烁
      bool on = ((now / BLINK_INTERVAL) % 2 == 0);
      setLights(false, on, false);

      if (elapsed >= TIME_YELLOW) {
        // 切换到红灯，检查行人请求
        if (pedestrianReq) {
          pedestrianReq    = false;
          pedestrianActive = true;
          Serial.println("[INFO] Pedestrian crossing activated!");
          // 三短鸣提示
          for (int i = 0; i < 3; i++) {
            beepOnce(1000, 100);
            delay(200);
          }
        }

        currentState   = STATE_RED;
        setLights(true, false, false);
        stateStartTime = millis();
        printState("RED", pedestrianActive ? TIME_PEDESTRIAN : TIME_RED);
      }
      break;
    }
  }

  // 绿灯期间收到行人请求，打印提示（不立即响应，等到下一个红灯）
  if (pedestrianReq && currentState == STATE_GREEN) {
    static bool printed = false;
    if (!printed) {
      Serial.println("[INFO] Pedestrian request queued (will activate next red)");
      printed = true;
    }
  } else {
    // 重置打印标志
  }

  delay(20);
}