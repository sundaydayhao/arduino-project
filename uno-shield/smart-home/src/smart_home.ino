/**
 * ============================================================
 * Arduino 智能家居中控 Shield
 * Smart Home Controller Shield
 * ============================================================
 *
 * 功能 / Features:
 *   - DHT11 温湿度传感器监测
 *   - PIR 人体感应（中断驱动）
 *   - LDR 光照检测
 *   - 继电器 1：灯光控制（自动/手动）
 *   - 继电器 2：风扇/电器控制（温度联动）
 *   - 无源蜂鸣器报警
 *   - OLED SSD1306 实时状态显示
 *   - 三种工作模式：AUTO / MANUAL / AWAY
 *   - 串口命令控制
 *
 * 工作模式说明 / Modes:
 *   AUTO   自动模式：光暗+有人→开灯，温度高→开风扇
 *   MANUAL 手动模式：继电器由串口命令控制
 *   AWAY   离家模式：有人触发→蜂鸣器报警
 *
 * 硬件连接 / Wiring:
 *   DHT11 数据脚    -> Pin 4   (另一端 VCC/GND，数据脚串 10kΩ 上拉到 VCC)
 *   PIR 输出        -> Pin 2   (外部中断 INT0，高电平有效)
 *   LDR 分压输出    -> A0      (LDR 上拉 10kΩ 到 VCC，下端接 GND)
 *   继电器 1 (灯)   -> Pin 7   (低电平触发模块接 LOW，高电平触发接 HIGH)
 *   继电器 2 (风扇) -> Pin 8   (同上)
 *   蜂鸣器          -> Pin 9   (无源蜂鸣器，另一端接 GND)
 *   OLED SDA        -> A4
 *   OLED SCL        -> A5
 *
 * 所需库 / Required Libraries:
 *   - DHT sensor library (by Adafruit)
 *   - Adafruit Unified Sensor (依赖库)
 *   - U8g2 (by olikraus)
 *
 * 串口命令 / Serial Commands (9600 baud):
 *   M<0/1/2>   切换模式 (0=AUTO, 1=MANUAL, 2=AWAY)
 *   R1<0/1>    手动控制继电器1 (仅MANUAL模式)
 *   R2<0/1>    手动控制继电器2 (仅MANUAL模式)
 *   T<25>      设置风扇启动温度阈值（如 T28）
 *   L<300>     设置开灯光照阈值（如 L400，越小越暗）
 *   A          切换报警开关（AWAY模式）
 *   S          打印当前所有状态
 * ============================================================
 */

#include <DHT.h>
#include <U8g2lib.h>
#include <Wire.h>

// ─── 引脚定义 / Pin Definitions ──────────────────────────────
#define PIN_DHT       4    // DHT11 数据引脚
#define PIN_PIR       2    // PIR 传感器（INT0）
#define PIN_LDR      A0    // 光敏电阻
#define PIN_RELAY1    7    // 继电器1（灯光）
#define PIN_RELAY2    8    // 继电器2（风扇）
#define PIN_BUZZER    9    // 无源蜂鸣器

// ─── 配置参数 / Configuration ─────────────────────────────────
#define DHT_TYPE          DHT11
#define RELAY_ON          LOW    // 低电平触发继电器（常见模块）
#define RELAY_OFF         HIGH

#define DEFAULT_TEMP_THRESHOLD   28.0f   // 风扇启动温度（°C）
#define DEFAULT_LIGHT_THRESHOLD  400     // 开灯光照阈值（ADC值，越小越暗）
#define PIR_TIMEOUT_MS           10000UL // 人体感应超时（10秒无人则清除）
#define DHT_READ_INTERVAL_MS     3000UL  // DHT 读取间隔
#define OLED_UPDATE_MS           1000UL  // OLED 刷新间隔
#define ALARM_BEEP_INTERVAL_MS   500UL   // 报警蜂鸣间隔

// ─── 工作模式 / Operating Modes ──────────────────────────────
enum Mode : uint8_t {
  MODE_AUTO   = 0,
  MODE_MANUAL = 1,
  MODE_AWAY   = 2
};

const char* modeNames[] = {"AUTO", "MANUAL", "AWAY"};

// ─── 对象实例化 / Object Instantiation ───────────────────────
DHT dht(PIN_DHT, DHT_TYPE);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ─── 全局状态 / Global State ──────────────────────────────────
Mode          currentMode       = MODE_AUTO;
float         temperature       = 0.0f;
float         humidity          = 0.0f;
int           lightLevel        = 0;       // ADC 0-1023（越小越暗）
bool          motionDetected    = false;
bool          relay1State       = false;   // true = ON
bool          relay2State       = false;
bool          alarmActive       = false;   // AWAY 模式报警状态
bool          dhtOK             = false;

float         tempThreshold     = DEFAULT_TEMP_THRESHOLD;
int           lightThreshold    = DEFAULT_LIGHT_THRESHOLD;

unsigned long lastMotionTime    = 0;
unsigned long lastDhtRead       = 0;
unsigned long lastOledUpdate    = 0;
unsigned long lastAlarmBeep     = 0;

volatile bool pirTriggered      = false;   // 中断标志

// ─── 中断服务程序 / ISR ───────────────────────────────────────
void ISR_PIR() {
  pirTriggered = true;
}

// ─── 继电器控制 / Relay Control ──────────────────────────────
void setRelay1(bool on) {
  relay1State = on;
  digitalWrite(PIN_RELAY1, on ? RELAY_ON : RELAY_OFF);
}

void setRelay2(bool on) {
  relay2State = on;
  digitalWrite(PIN_RELAY2, on ? RELAY_ON : RELAY_OFF);
}

// ─── 蜂鸣器 / Buzzer ─────────────────────────────────────────
void beep(int freq, int durationMs) {
  tone(PIN_BUZZER, freq, durationMs);
}

void beepOff() {
  noTone(PIN_BUZZER);
}

// ─── 读取传感器 / Read Sensors ────────────────────────────────
void readSensors() {
  unsigned long now = millis();

  // DHT11（每 3 秒读一次，避免过于频繁）
  if (now - lastDhtRead >= DHT_READ_INTERVAL_MS) {
    lastDhtRead = now;
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      temperature = t;
      humidity    = h;
      dhtOK       = true;
    } else {
      dhtOK = false;
    }
  }

  // LDR（每次 loop 读取）
  lightLevel = analogRead(PIN_LDR);

  // PIR（中断触发）
  if (pirTriggered) {
    pirTriggered    = false;
    motionDetected  = true;
    lastMotionTime  = now;
    Serial.println(F("[PIR] Motion detected!"));
  }

  // PIR 超时清除
  if (motionDetected && (now - lastMotionTime >= PIR_TIMEOUT_MS)) {
    motionDetected = false;
    Serial.println(F("[PIR] Motion cleared."));
  }
}

// ─── 自动控制逻辑 / Auto Control Logic ───────────────────────
void autoControl() {
  if (currentMode != MODE_AUTO) return;

  // 灯光控制：光照暗 + 有人 → 开灯
  bool shouldLight = (lightLevel < lightThreshold) && motionDetected;
  if (shouldLight != relay1State) {
    setRelay1(shouldLight);
    Serial.print(F("[AUTO] Light "));
    Serial.println(shouldLight ? F("ON") : F("OFF"));
  }

  // 风扇控制：温度超过阈值 → 开风扇
  if (dhtOK) {
    bool shouldFan = (temperature >= tempThreshold);
    if (shouldFan != relay2State) {
      setRelay2(shouldFan);
      Serial.print(F("[AUTO] Fan "));
      Serial.print(shouldFan ? F("ON") : F("OFF"));
      Serial.print(F(" (Temp="));
      Serial.print(temperature, 1);
      Serial.println(F("C)"));
    }
  }
}

// ─── 离家模式报警 / Away Mode Alarm ──────────────────────────
void awayModeCheck() {
  if (currentMode != MODE_AWAY) return;

  if (motionDetected) {
    unsigned long now = millis();
    if (now - lastAlarmBeep >= ALARM_BEEP_INTERVAL_MS) {
      lastAlarmBeep = now;
      alarmActive   = true;
      beep(1200, 200);
      Serial.println(F("[ALARM] Intruder detected!"));
    }
  } else {
    if (alarmActive) {
      alarmActive = false;
      beepOff();
      Serial.println(F("[ALARM] Cleared."));
    }
  }
}

// ─── OLED 显示 / OLED Display ─────────────────────────────────
void updateOLED() {
  u8g2.clearBuffer();

  // ── 标题栏：模式
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "Smart Home");

  // 模式标签（右上角）
  const char* mLabel = modeNames[currentMode];
  u8g2.drawStr(128 - strlen(mLabel) * 6, 10, mLabel);

  // 分隔线
  u8g2.drawHLine(0, 12, 128);

  // ── 温湿度
  char buf[24];
  u8g2.setFont(u8g2_font_6x10_tf);
  if (dhtOK) {
    sprintf(buf, "T:%.1fC  H:%.0f%%", temperature, humidity);
  } else {
    strcpy(buf, "T:--.-C  H:--%%");
  }
  u8g2.drawStr(0, 24, buf);

  // ── 光照 + 人体
  sprintf(buf, "Lux:%4d  %s", lightLevel, motionDetected ? "MOV!" : "----");
  u8g2.drawStr(0, 36, buf);

  // ── 继电器状态
  sprintf(buf, "R1:%s  R2:%s",
          relay1State ? "ON " : "OFF",
          relay2State ? "ON " : "OFF");
  u8g2.drawStr(0, 48, buf);

  // ── 底部状态栏
  u8g2.setFont(u8g2_font_5x7_tf);
  if (currentMode == MODE_AWAY && alarmActive) {
    u8g2.drawStr(0, 62, "!! ALARM TRIGGERED !!");
  } else if (currentMode == MODE_AUTO) {
    sprintf(buf, "T>%.0fC:Fan  Lux<%d:Light", tempThreshold, lightThreshold);
    u8g2.drawStr(0, 62, buf);
  } else {
    u8g2.drawStr(0, 62, "Manual mode - use serial");
  }

  u8g2.sendBuffer();
}

// ─── 串口命令处理 / Serial Command Handler ────────────────────
void handleSerial() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd.length() == 0) return;

  char c = cmd.charAt(0);

  // M<0/1/2> 切换模式
  if (c == 'M' || c == 'm') {
    int m = cmd.substring(1).toInt();
    if (m >= 0 && m <= 2) {
      currentMode = (Mode)m;
      alarmActive = false;
      beepOff();
      // 切换到手动模式时关闭所有继电器
      if (currentMode == MODE_MANUAL) {
        setRelay1(false);
        setRelay2(false);
      }
      Serial.print(F("[MODE] "));
      Serial.println(modeNames[currentMode]);
    } else {
      Serial.println(F("[ERR] Mode: M0=AUTO M1=MANUAL M2=AWAY"));
    }
    return;
  }

  // R1<0/1> 继电器1
  if ((c == 'R' || c == 'r') && cmd.length() >= 3 && cmd.charAt(1) == '1') {
    if (currentMode != MODE_MANUAL) {
      Serial.println(F("[ERR] Switch to MANUAL mode first (M1)"));
      return;
    }
    bool on = (cmd.charAt(2) == '1');
    setRelay1(on);
    Serial.print(F("[R1] ")); Serial.println(on ? F("ON") : F("OFF"));
    return;
  }

  // R2<0/1> 继电器2
  if ((c == 'R' || c == 'r') && cmd.length() >= 3 && cmd.charAt(1) == '2') {
    if (currentMode != MODE_MANUAL) {
      Serial.println(F("[ERR] Switch to MANUAL mode first (M1)"));
      return;
    }
    bool on = (cmd.charAt(2) == '1');
    setRelay2(on);
    Serial.print(F("[R2] ")); Serial.println(on ? F("ON") : F("OFF"));
    return;
  }

  // T<value> 设置温度阈值
  if (c == 'T' || c == 't') {
    float t = cmd.substring(1).toFloat();
    if (t >= 10.0f && t <= 50.0f) {
      tempThreshold = t;
      Serial.print(F("[THRESHOLD] Fan temp: "));
      Serial.print(tempThreshold, 1);
      Serial.println(F("C"));
    } else {
      Serial.println(F("[ERR] Temp threshold: 10-50"));
    }
    return;
  }

  // L<value> 设置光照阈值
  if (c == 'L' || c == 'l') {
    int lv = cmd.substring(1).toInt();
    if (lv >= 50 && lv <= 1000) {
      lightThreshold = lv;
      Serial.print(F("[THRESHOLD] Light: "));
      Serial.println(lightThreshold);
    } else {
      Serial.println(F("[ERR] Light threshold: 50-1000"));
    }
    return;
  }

  // A 切换报警（AWAY模式）
  if (c == 'A' || c == 'a') {
    if (currentMode == MODE_AWAY) {
      alarmActive = false;
      beepOff();
      Serial.println(F("[ALARM] Silenced."));
    } else {
      Serial.println(F("[INFO] Switch to AWAY mode first (M2)"));
    }
    return;
  }

  // S 打印状态
  if (c == 'S' || c == 's') {
    Serial.println(F("=== Smart Home Status ==="));
    Serial.print(F("Mode: ")); Serial.println(modeNames[currentMode]);
    Serial.print(F("Temp: "));
    if (dhtOK) { Serial.print(temperature, 1); Serial.println(F("C")); }
    else Serial.println(F("N/A"));
    Serial.print(F("Humidity: "));
    if (dhtOK) { Serial.print(humidity, 0); Serial.println(F("%")); }
    else Serial.println(F("N/A"));
    Serial.print(F("Light(ADC): ")); Serial.println(lightLevel);
    Serial.print(F("Motion: ")); Serial.println(motionDetected ? F("YES") : F("NO"));
    Serial.print(F("Relay1(Light): ")); Serial.println(relay1State ? F("ON") : F("OFF"));
    Serial.print(F("Relay2(Fan):   ")); Serial.println(relay2State ? F("ON") : F("OFF"));
    Serial.print(F("Temp threshold: ")); Serial.print(tempThreshold, 1); Serial.println(F("C"));
    Serial.print(F("Light threshold: ")); Serial.println(lightThreshold);
    return;
  }

  // 帮助
  Serial.println(F("=== Commands ==="));
  Serial.println(F("M0/M1/M2   Mode: AUTO / MANUAL / AWAY"));
  Serial.println(F("R10/R11    Relay1 OFF/ON  (MANUAL only)"));
  Serial.println(F("R20/R21    Relay2 OFF/ON  (MANUAL only)"));
  Serial.println(F("T<val>     Fan temp threshold (e.g. T28)"));
  Serial.println(F("L<val>     Light threshold (e.g. L400)"));
  Serial.println(F("A          Silence alarm (AWAY mode)"));
  Serial.println(F("S          Print status"));
}

// ─── Setup ────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  Serial.println(F("=== Smart Home Controller ==="));

  // 引脚初始化
  pinMode(PIN_PIR,    INPUT);
  pinMode(PIN_RELAY1, OUTPUT);
  pinMode(PIN_RELAY2, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  // 继电器初始关闭
  setRelay1(false);
  setRelay2(false);

  // PIR 中断（上升沿触发）
  attachInterrupt(digitalPinToInterrupt(PIN_PIR), ISR_PIR, RISING);

  // 初始化 DHT
  dht.begin();

  // 初始化 OLED
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.clearBuffer();
  u8g2.drawStr(10, 20, "Smart Home");
  u8g2.drawStr(10, 35, "Initializing...");
  u8g2.sendBuffer();
  delay(1000);

  // 开机提示音
  beep(1000, 100); delay(150);
  beep(1500, 100); delay(150);
  beepOff();

  Serial.println(F("Ready. Send '?' for help."));
  Serial.print(F("Mode: ")); Serial.println(modeNames[currentMode]);
  Serial.print(F("Fan threshold: ")); Serial.print(tempThreshold, 1); Serial.println(F("C"));
  Serial.print(F("Light threshold: ")); Serial.println(lightThreshold);
}

// ─── Loop ─────────────────────────────────────────────────────
void loop() {
  readSensors();
  autoControl();
  awayModeCheck();
  handleSerial();

  // OLED 定时刷新
  if (millis() - lastOledUpdate >= OLED_UPDATE_MS) {
    lastOledUpdate = millis();
    updateOLED();

    // 串口定时输出（每 5 秒）
    static unsigned long lastSerialPrint = 0;
    if (millis() - lastSerialPrint >= 5000) {
      lastSerialPrint = millis();
      if (dhtOK) {
        Serial.print(F("[DATA] T="));
        Serial.print(temperature, 1);
        Serial.print(F("C H="));
        Serial.print(humidity, 0);
        Serial.print(F("% Lux="));
        Serial.print(lightLevel);
        Serial.print(F(" PIR="));
        Serial.print(motionDetected ? F("1") : F("0"));
        Serial.print(F(" R1="));
        Serial.print(relay1State ? F("1") : F("0"));
        Serial.print(F(" R2="));
        Serial.println(relay2State ? F("1") : F("0"));
      }
    }
  }
}