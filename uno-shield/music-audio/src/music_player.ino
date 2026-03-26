/**
 * ============================================================
 * Arduino DFPlayer Mini MP3 播放器 Shield
 * DFPlayer Mini MP3 Player Shield
 * ============================================================
 *
 * 功能 / Features:
 *   - DFPlayer Mini 播放 SD 卡中的 MP3/WAV 文件
 *   - OLED SSD1306 显示曲目编号、音量、播放状态、进度条动画
 *   - 5 个按键：播放/暂停、下一曲、上一曲、音量+、音量-
 *   - 电位器模拟音量控制
 *   - LED 随播放状态指示
 *   - 串口命令控制
 *
 * 硬件连接 / Wiring:
 *   DFPlayer Mini:
 *     VCC  -> 5V
 *     GND  -> GND
 *     TX   -> Pin 10 (SoftwareSerial RX)
 *     RX   -> Pin 11 (SoftwareSerial TX，串联 1kΩ 电阻)
 *     SPK1 -> 扬声器正极
 *     SPK2 -> 扬声器负极
 *
 *   OLED SSD1306 (I2C):
 *     VCC -> 3.3V 或 5V
 *     GND -> GND
 *     SDA -> A4
 *     SCL -> A5
 *
 *   按键（另一端接 GND，使用内部上拉）:
 *     播放/暂停 -> Pin 2
 *     下一曲    -> Pin 3
 *     上一曲    -> Pin 4
 *     音量+     -> Pin 5
 *     音量-     -> Pin 6
 *
 *   电位器（音量）:
 *     中间脚   -> A0
 *     两端     -> 5V 和 GND
 *
 *   LED 状态指示:
 *     Pin 13 -> 220Ω -> LED -> GND
 *
 * SD 卡文件命名规则 / SD Card File Naming:
 *   文件必须放在 SD 卡根目录，命名为：
 *   0001.mp3, 0002.mp3, 0003.mp3 ...
 *   或放在 /01/ 文件夹中：/01/001.mp3
 *
 * 所需库 / Required Libraries:
 *   - DFRobotDFPlayerMini (搜索 "DFRobotDFPlayerMini")
 *   - U8g2 (by olikraus)
 *   - SoftwareSerial (Arduino 内置)
 *
 * 串口命令 / Serial Commands (9600 baud):
 *   P    播放/暂停
 *   N    下一曲
 *   B    上一曲
 *   S    停止
 *   V0-30  设置音量（如 V15）
 *   T    打印当前状态
 * ============================================================
 */

#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <U8g2lib.h>
#include <Wire.h>

// ─── 引脚定义 / Pin Definitions ──────────────────────────────
#define PIN_DF_RX     10   // 接 DFPlayer TX
#define PIN_DF_TX     11   // 接 DFPlayer RX（串联 1kΩ）
#define PIN_BTN_PLAY   2
#define PIN_BTN_NEXT   3
#define PIN_BTN_PREV   4
#define PIN_BTN_VOLUP  5
#define PIN_BTN_VOLDN  6
#define PIN_POT_VOL   A0   // 电位器音量
#define PIN_LED       13

// ─── 配置参数 / Configuration ─────────────────────────────────
#define VOLUME_DEFAULT   15    // 默认音量 (0-30)
#define VOLUME_MAX       30
#define VOLUME_MIN        0
#define TOTAL_TRACKS     10    // SD 卡中的曲目总数（根据实际修改）
#define DEBOUNCE_MS     200
#define OLED_UPDATE_MS  500    // OLED 刷新间隔

// ─── 对象实例化 / Object Instantiation ───────────────────────
SoftwareSerial dfSerial(PIN_DF_RX, PIN_DF_TX);
DFRobotDFPlayerMini dfPlayer;
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ─── 全局状态 / Global State ──────────────────────────────────
int  currentTrack   = 1;
int  currentVolume  = VOLUME_DEFAULT;
bool isPlaying      = false;
bool dfReady        = false;

// 按键去抖
unsigned long lastBtn[5] = {0, 0, 0, 0, 0};

// OLED 刷新
unsigned long lastOledUpdate = 0;

// 动画帧
uint8_t animFrame = 0;

// ─── OLED 显示 / OLED Display ─────────────────────────────────
void updateOLED() {
  u8g2.clearBuffer();

  // ── 标题栏
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "MP3 Player");

  // 播放状态图标
  if (isPlaying) {
    // 播放三角形
    u8g2.drawTriangle(90, 2, 90, 10, 98, 6);
    // 动画点
    for (int i = 0; i < 3; i++) {
      if ((animFrame / 2) % 3 >= i) {
        u8g2.drawDisc(104 + i * 6, 6, 2);
      }
    }
  } else {
    // 暂停双竖线
    u8g2.drawBox(90, 2, 3, 9);
    u8g2.drawBox(95, 2, 3, 9);
  }

  // 分隔线
  u8g2.drawHLine(0, 13, 128);

  // ── 曲目信息
  char buf[24];
  u8g2.setFont(u8g2_font_7x13B_tf);
  sprintf(buf, "Track: %02d / %02d", currentTrack, TOTAL_TRACKS);
  u8g2.drawStr(0, 30, buf);

  // ── 音量条
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 44, "Vol:");
  // 音量进度条（背景）
  u8g2.drawFrame(28, 36, 80, 10);
  // 音量进度条（填充）
  int volWidth = (int)(80.0 * currentVolume / VOLUME_MAX);
  if (volWidth > 0) u8g2.drawBox(28, 36, volWidth, 10);
  // 音量数字
  sprintf(buf, "%2d", currentVolume);
  u8g2.drawStr(112, 44, buf);

  // ── 底部提示
  u8g2.setFont(u8g2_font_5x7_tf);
  if (dfReady) {
    u8g2.drawStr(0, 58, "P=Play  N=Next  B=Prev");
  } else {
    u8g2.drawStr(0, 58, "! DFPlayer not found !");
  }

  u8g2.sendBuffer();
  animFrame++;
}

// ─── 按键处理 / Button Handling ───────────────────────────────
void handleButtons() {
  unsigned long now = millis();

  // 播放/暂停
  if (digitalRead(PIN_BTN_PLAY) == LOW && now - lastBtn[0] > DEBOUNCE_MS) {
    lastBtn[0] = now;
    if (!dfReady) return;
    if (isPlaying) {
      dfPlayer.pause();
      isPlaying = false;
      digitalWrite(PIN_LED, LOW);
      Serial.println(F("[PAUSE]"));
    } else {
      dfPlayer.start();
      isPlaying = true;
      digitalWrite(PIN_LED, HIGH);
      Serial.println(F("[PLAY]"));
    }
  }

  // 下一曲
  if (digitalRead(PIN_BTN_NEXT) == LOW && now - lastBtn[1] > DEBOUNCE_MS) {
    lastBtn[1] = now;
    if (!dfReady) return;
    currentTrack = (currentTrack % TOTAL_TRACKS) + 1;
    dfPlayer.play(currentTrack);
    isPlaying = true;
    digitalWrite(PIN_LED, HIGH);
    Serial.print(F("[NEXT] Track ")); Serial.println(currentTrack);
  }

  // 上一曲
  if (digitalRead(PIN_BTN_PREV) == LOW && now - lastBtn[2] > DEBOUNCE_MS) {
    lastBtn[2] = now;
    if (!dfReady) return;
    currentTrack = (currentTrack - 2 + TOTAL_TRACKS) % TOTAL_TRACKS + 1;
    dfPlayer.play(currentTrack);
    isPlaying = true;
    digitalWrite(PIN_LED, HIGH);
    Serial.print(F("[PREV] Track ")); Serial.println(currentTrack);
  }

  // 音量+
  if (digitalRead(PIN_BTN_VOLUP) == LOW && now - lastBtn[3] > DEBOUNCE_MS) {
    lastBtn[3] = now;
    currentVolume = min(VOLUME_MAX, currentVolume + 2);
    if (dfReady) dfPlayer.volume(currentVolume);
    Serial.print(F("[VOL+] ")); Serial.println(currentVolume);
  }

  // 音量-
  if (digitalRead(PIN_BTN_VOLDN) == LOW && now - lastBtn[4] > DEBOUNCE_MS) {
    lastBtn[4] = now;
    currentVolume = max(VOLUME_MIN, currentVolume - 2);
    if (dfReady) dfPlayer.volume(currentVolume);
    Serial.print(F("[VOL-] ")); Serial.println(currentVolume);
  }
}

// ─── 电位器音量控制 / Potentiometer Volume ────────────────────
void handlePotVolume() {
  static int lastPotVol = -1;
  int raw = analogRead(PIN_POT_VOL);
  int vol = map(raw, 0, 1023, 0, VOLUME_MAX);

  if (abs(vol - lastPotVol) >= 1) {
    lastPotVol    = vol;
    currentVolume = vol;
    if (dfReady) dfPlayer.volume(currentVolume);
  }
}

// ─── 串口命令处理 / Serial Command Handler ────────────────────
void handleSerial() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd.length() == 0) return;

  char c = cmd.charAt(0);

  if (c == 'P' || c == 'p') {
    if (!dfReady) { Serial.println(F("[ERR] DFPlayer not ready")); return; }
    if (isPlaying) { dfPlayer.pause(); isPlaying = false; Serial.println(F("[PAUSE]")); }
    else           { dfPlayer.start(); isPlaying = true;  Serial.println(F("[PLAY]")); }
    return;
  }
  if (c == 'N' || c == 'n') {
    if (!dfReady) return;
    currentTrack = (currentTrack % TOTAL_TRACKS) + 1;
    dfPlayer.play(currentTrack); isPlaying = true;
    Serial.print(F("[NEXT] ")); Serial.println(currentTrack);
    return;
  }
  if (c == 'B' || c == 'b') {
    if (!dfReady) return;
    currentTrack = (currentTrack - 2 + TOTAL_TRACKS) % TOTAL_TRACKS + 1;
    dfPlayer.play(currentTrack); isPlaying = true;
    Serial.print(F("[PREV] ")); Serial.println(currentTrack);
    return;
  }
  if (c == 'S' || c == 's') {
    if (!dfReady) return;
    dfPlayer.stop(); isPlaying = false; digitalWrite(PIN_LED, LOW);
    Serial.println(F("[STOP]"));
    return;
  }
  if (c == 'V' || c == 'v') {
    int vol = cmd.substring(1).toInt();
    vol = constrain(vol, 0, 30);
    currentVolume = vol;
    if (dfReady) dfPlayer.volume(currentVolume);
    Serial.print(F("[VOL] ")); Serial.println(currentVolume);
    return;
  }
  if (c == 'T' || c == 't') {
    Serial.println(F("=== Status ==="));
    Serial.print(F("Track: ")); Serial.print(currentTrack);
    Serial.print(F(" / ")); Serial.println(TOTAL_TRACKS);
    Serial.print(F("Volume: ")); Serial.println(currentVolume);
    Serial.print(F("Playing: ")); Serial.println(isPlaying ? "YES" : "NO");
    Serial.print(F("DFPlayer: ")); Serial.println(dfReady ? "OK" : "NOT FOUND");
    return;
  }

  Serial.println(F("Commands: P=Play/Pause N=Next B=Prev S=Stop V<0-30>=Volume T=Status"));
}

// ─── Setup ────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  dfSerial.begin(9600);

  Serial.println(F("=== DFPlayer Mini MP3 Player ==="));

  // 初始化引脚
  pinMode(PIN_BTN_PLAY,  INPUT_PULLUP);
  pinMode(PIN_BTN_NEXT,  INPUT_PULLUP);
  pinMode(PIN_BTN_PREV,  INPUT_PULLUP);
  pinMode(PIN_BTN_VOLUP, INPUT_PULLUP);
  pinMode(PIN_BTN_VOLDN, INPUT_PULLUP);
  pinMode(PIN_LED,       OUTPUT);

  // 初始化 OLED
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.clearBuffer();
  u8g2.drawStr(10, 20, "MP3 Player");
  u8g2.drawStr(10, 35, "Initializing...");
  u8g2.sendBuffer();

  // 初始化 DFPlayer
  Serial.println(F("Initializing DFPlayer..."));
  if (!dfPlayer.begin(dfSerial)) {
    dfReady = false;
    Serial.println(F("[WARN] DFPlayer not found!"));
    Serial.println(F("  Check: TX->Pin10, RX->Pin11(1kΩ), VCC->5V, GND->GND"));
    u8g2.clearBuffer();
    u8g2.drawStr(0, 20, "DFPlayer");
    u8g2.drawStr(0, 35, "NOT FOUND!");
    u8g2.drawStr(0, 50, "Check wiring");
    u8g2.sendBuffer();
  } else {
    dfReady = true;
    dfPlayer.volume(VOLUME_DEFAULT);
    dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
    Serial.println(F("[OK] DFPlayer ready!"));
    Serial.print(F("Files on SD: "));
    Serial.println(dfPlayer.readFileCounts());
  }

  Serial.println(F("Commands: P=Play/Pause N=Next B=Prev S=Stop V<0-30>=Volume T=Status"));
  updateOLED();
}

// ─── Loop ─────────────────────────────────────────────────────
void loop() {
  handleButtons();
  handlePotVolume();
  handleSerial();

  // DFPlayer 状态监听
  if (dfReady && dfPlayer.available()) {
    uint8_t type  = dfPlayer.readType();
    int     value = dfPlayer.read();

    if (type == DFPlayerPlayFinished) {
      // 自动播放下一曲
      currentTrack = (currentTrack % TOTAL_TRACKS) + 1;
      dfPlayer.play(currentTrack);
      Serial.print(F("[AUTO-NEXT] Track ")); Serial.println(currentTrack);
    } else if (type == DFPlayerError) {
      Serial.print(F("[DF-ERR] ")); Serial.println(value);
    }
  }

  // LED 闪烁（播放中）
  if (isPlaying) {
    digitalWrite(PIN_LED, (millis() / 500) % 2);
  }

  // OLED 定时刷新
  if (millis() - lastOledUpdate >= OLED_UPDATE_MS) {
    lastOledUpdate = millis();
    updateOLED();
  }
}