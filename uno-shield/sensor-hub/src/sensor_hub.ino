/**
 * Sensor Hub Shield - 多传感器数据采集扩展板
 * ============================================================
 * 硬件配置:
 *   DHT22        → Pin 4
 *   LDR光敏      → A0 (10kΩ下拉)
 *   声音传感器   → A1 (模拟)
 *   电位器1      → A2 (RGB色相控制)
 *   电位器2      → A3 (亮度/速度控制)
 *   按键1        → Pin 2 (INT0, 内部上拉)
 *   按键2        → Pin 3 (INT1, 内部上拉)
 *   按键3        → Pin 7 (轮询, 内部上拉)
 *   LED红        → Pin 13
 *   LED黄        → Pin 12
 *   LED绿        → Pin 11
 *   LED蓝        → Pin 10
 *   RGB_R        → Pin 9  (PWM)
 *   RGB_G        → Pin 6  (PWM)
 *   RGB_B        → Pin 5  (PWM)
 *   无源蜂鸣器   → Pin 8
 *   I2C扩展      → A4(SDA) / A5(SCL)
 *
 * 依赖库:
 *   DHT sensor library (Adafruit)
 *   Adafruit Unified Sensor
 *
 * 串口命令 (9600 baud):
 *   S  → 立即打印传感器数据
 *   M  → 切换LED演示模式
 *   H  → 帮助信息
 * ============================================================
 */

#include <DHT.h>

// ─── 引脚定义 ────────────────────────────────────────────────
#define PIN_DHT       4
#define PIN_LDR       A0
#define PIN_SOUND     A1
#define PIN_POT1      A2   // RGB色相
#define PIN_POT2      A3   // 亮度/速度

#define PIN_BTN1      2    // INT0
#define PIN_BTN2      3    // INT1
#define PIN_BTN3      7

#define PIN_LED_RED   13
#define PIN_LED_YEL   12
#define PIN_LED_GRN   11
#define PIN_LED_BLU   10

#define PIN_RGB_R     9
#define PIN_RGB_G     6
#define PIN_RGB_B     5

#define PIN_BUZZER    8

// ─── DHT 配置 ────────────────────────────────────────────────
#define DHT_TYPE      DHT22
DHT dht(PIN_DHT, DHT_TYPE);

// ─── LED 演示模式 ────────────────────────────────────────────
enum LedMode {
  MODE_OFF = 0,
  MODE_CHASE,
  MODE_BREATHE,
  MODE_ALL_ON,
  MODE_COUNT
};

// ─── 音符频率 ────────────────────────────────────────────────
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523

// ─── 全局变量 ────────────────────────────────────────────────
volatile bool btn1Pressed = false;   // 切换LED模式
volatile bool btn2Pressed = false;   // 播放旋律
bool          btn3Last    = HIGH;    // 自动光控切换

LedMode  ledMode      = MODE_OFF;
bool     autoLight    = false;       // LDR自动控制LED亮度

// 传感器读取间隔
unsigned long lastSensorTime = 0;
const unsigned long SENSOR_INTERVAL = 2000;

// LED追逐动画
unsigned long lastChaseTime = 0;
uint8_t       chaseIndex    = 0;
const uint8_t chasePins[]   = {PIN_LED_RED, PIN_LED_YEL, PIN_LED_GRN, PIN_LED_BLU};

// 呼吸灯
unsigned long lastBreatheTime = 0;
int  breatheVal  = 0;
int  breatheDir  = 1;

// 旋律播放
bool     melodyPlaying  = false;
uint8_t  melodyIndex    = 0;
unsigned long melodyNoteStart = 0;

const int melody[] = {
  NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4,
  NOTE_A4, NOTE_A4, NOTE_G4, 0,
  NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4,
  NOTE_D4, NOTE_D4, NOTE_C4, 0
};
const int melodyDur[] = {
  200, 200, 200, 200,
  200, 200, 400, 200,
  200, 200, 200, 200,
  200, 200, 400, 400
};
const uint8_t MELODY_LEN = sizeof(melody) / sizeof(melody[0]);

// 按键防抖
unsigned long btn1DebounceTime = 0;
unsigned long btn2DebounceTime = 0;
const unsigned long DEBOUNCE_MS = 50;

// ─── 中断服务程序 ────────────────────────────────────────────
void ISR_btn1() {
  if (millis() - btn1DebounceTime > DEBOUNCE_MS) {
    btn1Pressed = true;
    btn1DebounceTime = millis();
  }
}

void ISR_btn2() {
  if (millis() - btn2DebounceTime > DEBOUNCE_MS) {
    btn2Pressed = true;
    btn2DebounceTime = millis();
  }
}

// ─── HSV → RGB 转换 ─────────────────────────────────────────
// h: 0-359, s: 0-255, v: 0-255
void hsvToRgb(int h, uint8_t s, uint8_t v,
              uint8_t &r, uint8_t &g, uint8_t &b) {
  if (s == 0) { r = g = b = v; return; }
  uint8_t region  = h / 60;
  uint8_t remain  = (h % 60) * 255 / 60;
  uint8_t p = (uint32_t)v * (255 - s) / 255;
  uint8_t q = (uint32_t)v * (255 - ((uint32_t)s * remain / 255)) / 255;
  uint8_t t = (uint32_t)v * (255 - ((uint32_t)s * (255 - remain) / 255)) / 255;
  switch (region) {
    case 0: r=v; g=t; b=p; break;
    case 1: r=q; g=v; b=p; break;
    case 2: r=p; g=v; b=t; break;
    case 3: r=p; g=q; b=v; break;
    case 4: r=t; g=p; b=v; break;
    default:r=v; g=p; b=q; break;
  }
}

// ─── 设置 RGB LED ────────────────────────────────────────────
void setRGB(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(PIN_RGB_R, r);
  analogWrite(PIN_RGB_G, g);
  analogWrite(PIN_RGB_B, b);
}

// ─── 关闭所有单色LED ─────────────────────────────────────────
void allLedsOff() {
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_YEL, LOW);
  digitalWrite(PIN_LED_GRN, LOW);
  digitalWrite(PIN_LED_BLU, LOW);
}

// ─── 打印传感器数据 ──────────────────────────────────────────
void printSensors() {
  float temp = dht.readTemperature();
  float humi = dht.readHumidity();
  int   ldr  = analogRead(PIN_LDR);
  int   snd  = analogRead(PIN_SOUND);
  int   pot1 = analogRead(PIN_POT1);
  int   pot2 = analogRead(PIN_POT2);

  Serial.println(F("─────────────────────────────"));
  if (isnan(temp) || isnan(humi)) {
    Serial.println(F("[DHT22] 读取失败"));
  } else {
    Serial.print(F("[DHT22] 温度: "));
    Serial.print(temp, 1);
    Serial.print(F(" °C  湿度: "));
    Serial.print(humi, 1);
    Serial.println(F(" %"));
  }
  Serial.print(F("[LDR  ] 光照: "));
  Serial.print(ldr);
  Serial.print(F("  ("));
  Serial.print(map(ldr, 0, 1023, 0, 100));
  Serial.println(F("%)"));

  Serial.print(F("[声音 ] 强度: "));
  Serial.println(snd);

  Serial.print(F("[POT1 ] 色相: "));
  Serial.print(map(pot1, 0, 1023, 0, 359));
  Serial.println(F("°"));

  Serial.print(F("[POT2 ] 速度: "));
  Serial.println(map(pot2, 0, 1023, 0, 100));
  Serial.println(F("─────────────────────────────"));
}

// ─── 打印帮助 ────────────────────────────────────────────────
void printHelp() {
  Serial.println(F("=== Sensor Hub 串口命令 ==="));
  Serial.println(F("  S  → 立即打印传感器数据"));
  Serial.println(F("  M  → 切换LED演示模式"));
  Serial.println(F("       (关闭→追逐→呼吸→全亮)"));
  Serial.println(F("  H  → 显示帮助"));
  Serial.println(F("按键1 → 切换LED模式"));
  Serial.println(F("按键2 → 播放/停止旋律"));
  Serial.println(F("按键3 → 切换LDR自动光控"));
  Serial.println(F("POT1  → RGB色相 (0-359°)"));
  Serial.println(F("POT2  → 动画速度/亮度"));
  Serial.println(F("=========================="));
}

// ─── 旋律播放（非阻塞）─────────────────────────────────────
void updateMelody() {
  if (!melodyPlaying) return;
  unsigned long now = millis();
  if (now - melodyNoteStart >= (unsigned long)melodyDur[melodyIndex]) {
    noTone(PIN_BUZZER);
    melodyIndex++;
    if (melodyIndex >= MELODY_LEN) {
      melodyIndex   = 0;
      melodyPlaying = false;
      return;
    }
    melodyNoteStart = now;
    if (melody[melodyIndex] > 0) {
      tone(PIN_BUZZER, melody[melodyIndex]);
    }
  }
}

// ─── LED 追逐动画（非阻塞）──────────────────────────────────
void updateChase() {
  int pot2 = analogRead(PIN_POT2);
  unsigned long interval = map(pot2, 0, 1023, 80, 500);
  if (millis() - lastChaseTime < interval) return;
  lastChaseTime = millis();
  allLedsOff();
  digitalWrite(chasePins[chaseIndex], HIGH);
  chaseIndex = (chaseIndex + 1) % 4;
}

// ─── LED 呼吸灯（非阻塞）────────────────────────────────────
void updateBreathe() {
  int pot2 = analogRead(PIN_POT2);
  unsigned long interval = map(pot2, 0, 1023, 5, 30);
  if (millis() - lastBreatheTime < interval) return;
  lastBreatheTime = millis();
  breatheVal += breatheDir * 3;
  if (breatheVal >= 255) { breatheVal = 255; breatheDir = -1; }
  if (breatheVal <= 0)   { breatheVal = 0;   breatheDir =  1; }
  analogWrite(PIN_LED_BLU, breatheVal);
  analogWrite(PIN_RGB_B,   breatheVal);
}

// ─── setup ───────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);

  // 单色LED
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_YEL, OUTPUT);
  pinMode(PIN_LED_GRN, OUTPUT);
  pinMode(PIN_LED_BLU, OUTPUT);
  allLedsOff();

  // RGB LED
  pinMode(PIN_RGB_R, OUTPUT);
  pinMode(PIN_RGB_G, OUTPUT);
  pinMode(PIN_RGB_B, OUTPUT);
  setRGB(0, 0, 0);

  // 蜂鸣器
  pinMode(PIN_BUZZER, OUTPUT);
  noTone(PIN_BUZZER);

  // 按键（内部上拉）
  pinMode(PIN_BTN1, INPUT_PULLUP);
  pinMode(PIN_BTN2, INPUT_PULLUP);
  pinMode(PIN_BTN3, INPUT_PULLUP);

  // 外部中断
  attachInterrupt(digitalPinToInterrupt(PIN_BTN1), ISR_btn1, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_BTN2), ISR_btn2, FALLING);

  // DHT22
  dht.begin();

  Serial.println(F("=== Sensor Hub Shield 启动 ==="));
  printHelp();
}

// ─── loop ────────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  // ── 定时读取传感器 ──────────────────────────────────────
  if (now - lastSensorTime >= SENSOR_INTERVAL) {
    lastSensorTime = now;
    printSensors();
  }

  // ── 按键1：切换LED模式 ──────────────────────────────────
  if (btn1Pressed) {
    btn1Pressed = false;
    ledMode = (LedMode)((ledMode + 1) % MODE_COUNT);
    allLedsOff();
    setRGB(0, 0, 0);
    noTone(PIN_BUZZER);
    const char* modeNames[] = {"关闭", "追逐", "呼吸", "全亮"};
    Serial.print(F("[LED] 模式切换 → "));
    Serial.println(modeNames[ledMode]);
  }

  // ── 按键2：播放/停止旋律 ────────────────────────────────
  if (btn2Pressed) {
    btn2Pressed = false;
    if (melodyPlaying) {
      melodyPlaying = false;
      melodyIndex   = 0;
      noTone(PIN_BUZZER);
      Serial.println(F("[蜂鸣器] 停止"));
    } else {
      melodyPlaying   = true;
      melodyIndex     = 0;
      melodyNoteStart = millis();
      tone(PIN_BUZZER, melody[0]);
      Serial.println(F("[蜂鸣器] 播放旋律"));
    }
  }

  // ── 按键3：切换自动光控（轮询）─────────────────────────
  bool btn3Now = digitalRead(PIN_BTN3);
  if (btn3Now == LOW && btn3Last == HIGH) {
    delay(20); // 简单防抖
    if (digitalRead(PIN_BTN3) == LOW) {
      autoLight = !autoLight;
      Serial.print(F("[自动光控] "));
      Serial.println(autoLight ? F("开启") : F("关闭"));
    }
  }
  btn3Last = btn3Now;

  // ── RGB LED：POT1控制色相，POT2控制亮度 ─────────────────
  if (ledMode != MODE_BREATHE) {  // 呼吸模式时RGB单独处理
    int pot1 = analogRead(PIN_POT1);
    int pot2 = analogRead(PIN_POT2);
    int hue  = map(pot1, 0, 1023, 0, 359);
    uint8_t bright = map(pot2, 0, 1023, 10, 255);
    uint8_t r, g, b;
    hsvToRgb(hue, 255, bright, r, g, b);
    setRGB(r, g, b);
  }

  // ── 自动光控：LDR控制LED亮度 ────────────────────────────
  if (autoLight && ledMode == MODE_OFF) {
    int ldr = analogRead(PIN_LDR);
    // 光线越暗，LED越亮
    int brightness = map(ldr, 0, 1023, 255, 0);
    analogWrite(PIN_LED_GRN, brightness);
  }

  // ── LED 模式更新 ─────────────────────────────────────────
  switch (ledMode) {
    case MODE_OFF:
      if (!autoLight) allLedsOff();
      break;
    case MODE_CHASE:
      updateChase();
      break;
    case MODE_BREATHE:
      allLedsOff();  // 先关单色，再呼吸
      updateBreathe();
      break;
    case MODE_ALL_ON:
      digitalWrite(PIN_LED_RED, HIGH);
      digitalWrite(PIN_LED_YEL, HIGH);
      digitalWrite(PIN_LED_GRN, HIGH);
      digitalWrite(PIN_LED_BLU, HIGH);
      break;
    default:
      break;
  }

  // ── 旋律更新 ─────────────────────────────────────────────
  updateMelody();

  // ── 串口命令处理 ─────────────────────────────────────────
  if (Serial.available()) {
    char cmd = toupper(Serial.read());
    while (Serial.available()) Serial.read(); // 清空缓冲
    switch (cmd) {
      case 'S':
        printSensors();
        break;
      case 'M':
        btn1Pressed = true; // 复用按键1逻辑
        break;
      case 'H':
        printHelp();
        break;
      default:
        Serial.print(F("[错误] 未知命令: "));
        Serial.println(cmd);
        break;
    }
  }
}