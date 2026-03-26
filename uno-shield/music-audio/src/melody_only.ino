/**
 * ============================================================
 * Arduino 蜂鸣器旋律播放器（无需额外模块）
 * Buzzer Melody Player - No Extra Modules Required
 * ============================================================
 *
 * 功能 / Features:
 *   - 5 首内置旋律（生日歌、小星星、铃儿响叮当、欢乐颂、超级马里奥）
 *   - 非阻塞播放（loop() 不被 delay 阻塞）
 *   - 按键选曲、播放/暂停/停止、速度调节
 *   - LED 随节拍闪烁
 *   - 串口显示当前音符和旋律名称
 *
 * 硬件连接 / Wiring:
 *   无源蜂鸣器  -> Pin 8  (另一端接 GND)
 *   选曲按键    -> Pin 2  (另一端接 GND，内部上拉)
 *   播放/停止   -> Pin 3  (另一端接 GND，内部上拉)
 *   速度按键    -> Pin 4  (另一端接 GND，内部上拉)
 *   节拍 LED    -> Pin 13 (串联 220Ω 到 GND)
 *
 * 串口命令 / Serial Commands (9600 baud):
 *   1-5   选择旋律编号
 *   P     播放/暂停
 *   S     停止
 *   +     加速
 *   -     减速
 *   L     列出所有旋律
 *   H     帮助
 * ============================================================
 */

// ─── 音符频率定义 / Note Frequencies (Hz) ────────────────────
#define NOTE_REST  0
#define NOTE_B0    31
#define NOTE_C1    33
#define NOTE_D1    37
#define NOTE_E1    41
#define NOTE_F1    44
#define NOTE_G1    49
#define NOTE_A1    55
#define NOTE_B1    62
#define NOTE_C2    65
#define NOTE_D2    73
#define NOTE_E2    82
#define NOTE_F2    87
#define NOTE_G2    98
#define NOTE_A2   110
#define NOTE_B2   123
#define NOTE_C3   131
#define NOTE_D3   147
#define NOTE_E3   165
#define NOTE_F3   175
#define NOTE_G3   196
#define NOTE_A3   220
#define NOTE_B3   247
#define NOTE_C4   262   // 中央C / Middle C
#define NOTE_D4   294
#define NOTE_E4   330
#define NOTE_F4   349
#define NOTE_G4   392
#define NOTE_A4   440
#define NOTE_AS4  466   // A#4 / Bb4
#define NOTE_B4   494
#define NOTE_C5   523
#define NOTE_D5   587
#define NOTE_E5   659
#define NOTE_F5   698
#define NOTE_FS5  740   // F#5
#define NOTE_G5   784
#define NOTE_GS5  831   // G#5
#define NOTE_A5   880
#define NOTE_B5   988
#define NOTE_C6  1047

// ─── 引脚定义 / Pin Definitions ──────────────────────────────
#define PIN_BUZZER    8
#define PIN_BTN_SEL   2   // 选曲
#define PIN_BTN_PLAY  3   // 播放/停止
#define PIN_BTN_SPEED 4   // 速度
#define PIN_LED      13

// ─── 按键去抖 / Debounce ──────────────────────────────────────
#define DEBOUNCE_MS  200

// ─── 旋律数据 / Melody Data ───────────────────────────────────
// 时值编码：正数=标准, 负数=附点（×1.5）
// 1=全音符, 2=二分, 4=四分, 8=八分, 16=十六分

// ── 1. 生日快乐 Happy Birthday ────────────────────────────────
const int N_BIRTHDAY = 25;
const int notes_birthday[25] = {
  NOTE_C4, NOTE_C4, NOTE_D4, NOTE_C4, NOTE_F4, NOTE_E4,
  NOTE_C4, NOTE_C4, NOTE_D4, NOTE_C4, NOTE_G4, NOTE_F4,
  NOTE_C4, NOTE_C4, NOTE_C5, NOTE_A4, NOTE_F4, NOTE_E4, NOTE_D4,
  NOTE_AS4, NOTE_AS4, NOTE_A4, NOTE_F4, NOTE_G4, NOTE_F4
};
const int durs_birthday[25] = {
  4, 8, -4, -4, -4, -2,
  4, 8, -4, -4, -4, -2,
  4, 8, -4, -4, -4, -4, -4,
  4, 8, -4, -4, -4, -2
};

// ── 2. 小星星 Twinkle Twinkle Little Star ─────────────────────
const int N_TWINKLE = 42;
const int notes_twinkle[42] = {
  NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_G4,
  NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_C4,
  NOTE_G4, NOTE_G4, NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4,
  NOTE_G4, NOTE_G4, NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4,
  NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_G4,
  NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_C4
};
const int durs_twinkle[42] = {
  4, 4, 4, 4, 4, 4, 2,
  4, 4, 4, 4, 4, 4, 2,
  4, 4, 4, 4, 4, 4, 2,
  4, 4, 4, 4, 4, 4, 2,
  4, 4, 4, 4, 4, 4, 2,
  4, 4, 4, 4, 4, 4, 2
};

// ── 3. 铃儿响叮当 Jingle Bells ────────────────────────────────
const int N_JINGLE = 46;
const int notes_jingle[46] = {
  NOTE_E4, NOTE_E4, NOTE_E4,
  NOTE_E4, NOTE_E4, NOTE_E4,
  NOTE_E4, NOTE_G4, NOTE_C4, NOTE_D4, NOTE_E4,
  NOTE_F4, NOTE_F4, NOTE_F4, NOTE_F4,
  NOTE_F4, NOTE_E4, NOTE_E4, NOTE_E4, NOTE_E4,
  NOTE_E4, NOTE_D4, NOTE_D4, NOTE_E4, NOTE_D4, NOTE_G4,
  NOTE_E4, NOTE_E4, NOTE_E4,
  NOTE_E4, NOTE_E4, NOTE_E4,
  NOTE_E4, NOTE_G4, NOTE_C4, NOTE_D4, NOTE_E4,
  NOTE_F4, NOTE_F4, NOTE_F4, NOTE_F4,
  NOTE_G4, NOTE_G4, NOTE_F4, NOTE_E4, NOTE_D4, NOTE_C4
};
const int durs_jingle[46] = {
  4, 4, 2,
  4, 4, 2,
  4, 4, 4, 4, 1,
  4, 4, 4, 4,
  4, 4, 4, 8, 8,
  4, 4, 4, 4, 2, 2,
  4, 4, 2,
  4, 4, 2,
  4, 4, 4, 4, 1,
  4, 4, 4, 4,
  4, 4, 4, 4, 4, 1
};

// ── 4. 欢乐颂 Ode to Joy (Beethoven) ─────────────────────────
const int N_ODE = 64;
const int notes_ode[64] = {
  NOTE_E4, NOTE_E4, NOTE_F4, NOTE_G4,
  NOTE_G4, NOTE_F4, NOTE_E4, NOTE_D4,
  NOTE_C4, NOTE_C4, NOTE_D4, NOTE_E4,
  NOTE_E4, NOTE_D4, NOTE_D4,
  NOTE_E4, NOTE_E4, NOTE_F4, NOTE_G4,
  NOTE_G4, NOTE_F4, NOTE_E4, NOTE_D4,
  NOTE_C4, NOTE_C4, NOTE_D4, NOTE_E4,
  NOTE_D4, NOTE_C4, NOTE_C4,
  NOTE_D4, NOTE_D4, NOTE_E4, NOTE_C4,
  NOTE_D4, NOTE_E4, NOTE_F4, NOTE_E4, NOTE_C4,
  NOTE_D4, NOTE_E4, NOTE_F4, NOTE_E4, NOTE_D4,
  NOTE_C4, NOTE_D4, NOTE_G3,
  NOTE_E4, NOTE_E4, NOTE_F4, NOTE_G4,
  NOTE_G4, NOTE_F4, NOTE_E4, NOTE_D4,
  NOTE_C4, NOTE_C4, NOTE_D4, NOTE_E4,
  NOTE_D4, NOTE_C4, NOTE_C4
};
const int durs_ode[64] = {
  4, 4, 4, 4,
  4, 4, 4, 4,
  4, 4, 4, 4,
  -4, 8, 2,
  4, 4, 4, 4,
  4, 4, 4, 4,
  4, 4, 4, 4,
  -4, 8, 2,
  4, 4, 4, 4,
  4, 8, 8, 4, 4,
  4, 8, 8, 4, 4,
  4, 4, 2,
  4, 4, 4, 4,
  4, 4, 4, 4,
  4, 4, 4, 4,
  -4, 8, 2
};

// ── 5. 超级马里奥主题 Super Mario Bros Theme ──────────────────
const int N_MARIO = 60;
const int notes_mario[60] = {
  NOTE_E5, NOTE_E5, NOTE_REST, NOTE_E5, NOTE_REST, NOTE_C5, NOTE_E5,
  NOTE_G5, NOTE_REST, NOTE_G4, NOTE_REST,
  NOTE_C5, NOTE_REST, NOTE_G4, NOTE_REST, NOTE_E4,
  NOTE_A4, NOTE_REST, NOTE_B4, NOTE_REST, NOTE_AS4, NOTE_A4,
  NOTE_G4, NOTE_E5, NOTE_G5, NOTE_A5, NOTE_REST, NOTE_F5, NOTE_G5,
  NOTE_REST, NOTE_E5, NOTE_REST, NOTE_C5, NOTE_D5, NOTE_B4, NOTE_REST,
  NOTE_C5, NOTE_REST, NOTE_G4, NOTE_REST, NOTE_E4,
  NOTE_A4, NOTE_REST, NOTE_B4, NOTE_REST, NOTE_AS4, NOTE_A4,
  NOTE_G4, NOTE_E5, NOTE_G5, NOTE_A5, NOTE_REST, NOTE_F5, NOTE_G5,
  NOTE_REST, NOTE_E5, NOTE_REST, NOTE_C5, NOTE_D5, NOTE_B4, NOTE_REST
};
const int durs_mario[60] = {
  8, 8, 8, 8, 8, 8, 4,
  4, 4, 4, 4,
  -4, 8, -4, 8, -4,
  4, 8, 4, 8, 8, 4,
  8, 8, 8, 4, 8, 8, 4,
  8, 4, 8, 8, 8, 4, 8,
  -4, 8, -4, 8, -4,
  4, 8, 4, 8, 8, 4,
  8, 8, 8, 4, 8, 8, 4,
  8, 4, 8, 8, 8, 4, 8
};

// ─── 旋律索引表 / Melody Index ────────────────────────────────
const int*  melodyNotes[5] = {notes_birthday, notes_twinkle, notes_jingle, notes_ode, notes_mario};
const int*  melodyDurs[5]  = {durs_birthday,  durs_twinkle,  durs_jingle,  durs_ode,  durs_mario};
const int   melodyLens[5]  = {N_BIRTHDAY,     N_TWINKLE,     N_JINGLE,     N_ODE,     N_MARIO};
const char* melodyNames[5] = {
  "Happy Birthday",
  "Twinkle Twinkle",
  "Jingle Bells",
  "Ode to Joy",
  "Super Mario Bros"
};

// ─── 全局状态 / Global State ──────────────────────────────────
int           currentMelody  = 0;
int           currentNote    = 0;
bool          isPlaying      = false;
bool          isStopped      = true;
int           tempoFactor    = 100;   // 速度百分比（60=慢, 100=正常, 150=快）
int           wholeDuration  = 1000;  // 全音符时长（ms）

unsigned long lastBtnSel     = 0;
unsigned long lastBtnPlay    = 0;
unsigned long lastBtnSpeed   = 0;

unsigned long noteStartTime  = 0;
int           noteDuration   = 0;
bool          noteActive     = false;

// ─── 计算音符时长 / Calculate Note Duration ───────────────────
int calcDuration(int durCode) {
  int base = wholeDuration / abs(durCode);
  if (durCode < 0) base = base * 3 / 2;  // 附点音符 ×1.5
  return base;
}

// ─── 开始播放下一个音符 / Start Next Note ─────────────────────
void startNextNote() {
  if (currentNote >= melodyLens[currentMelody]) {
    isPlaying   = false;
    isStopped   = true;
    currentNote = 0;
    noTone(PIN_BUZZER);
    digitalWrite(PIN_LED, LOW);
    Serial.print(F("[END] "));
    Serial.println(melodyNames[currentMelody]);
    return;
  }

  int freq = melodyNotes[currentMelody][currentNote];
  int dur  = calcDuration(melodyDurs[currentMelody][currentNote]);

  Serial.print(F("["));
  Serial.print(currentNote + 1);
  Serial.print(F("/"));
  Serial.print(melodyLens[currentMelody]);
  Serial.print(F("] "));
  Serial.print(freq);
  Serial.print(F("Hz "));
  Serial.print(dur);
  Serial.println(F("ms"));

  if (freq == NOTE_REST) {
    noTone(PIN_BUZZER);
    digitalWrite(PIN_LED, LOW);
  } else {
    tone(PIN_BUZZER, freq, (int)(dur * 0.9));
    digitalWrite(PIN_LED, HIGH);
  }

  noteStartTime = millis();
  noteDuration  = dur;
  noteActive    = true;
  currentNote++;
}

// ─── 按键处理 / Button Handling ───────────────────────────────
void handleButtons() {
  unsigned long now = millis();

  // 选曲
  if (digitalRead(PIN_BTN_SEL) == LOW && now - lastBtnSel > DEBOUNCE_MS) {
    lastBtnSel  = now;
    isPlaying   = false;
    isStopped   = true;
    currentNote = 0;
    noTone(PIN_BUZZER);
    digitalWrite(PIN_LED, LOW);
    currentMelody = (currentMelody + 1) % 5;
    Serial.print(F("[SEL] "));
    Serial.print(currentMelody + 1);
    Serial.print(F(". "));
    Serial.println(melodyNames[currentMelody]);
  }

  // 播放/暂停/继续
  if (digitalRead(PIN_BTN_PLAY) == LOW && now - lastBtnPlay > DEBOUNCE_MS) {
    lastBtnPlay = now;
    if (isStopped) {
      isPlaying   = true;
      isStopped   = false;
      currentNote = 0;
      Serial.print(F("[PLAY] "));
      Serial.println(melodyNames[currentMelody]);
      startNextNote();
    } else if (isPlaying) {
      isPlaying = false;
      noTone(PIN_BUZZER);
      digitalWrite(PIN_LED, LOW);
      Serial.println(F("[PAUSE]"));
    } else {
      isPlaying = true;
      Serial.println(F("[RESUME]"));
      startNextNote();
    }
  }

  // 速度切换：慢(60%) → 正常(100%) → 快(150%) → 慢
  if (digitalRead(PIN_BTN_SPEED) == LOW && now - lastBtnSpeed > DEBOUNCE_MS) {
    lastBtnSpeed = now;
    if (tempoFactor < 80) {
      tempoFactor = 100;
    } else if (tempoFactor < 130) {
      tempoFactor = 150;
    } else {
      tempoFactor = 60;
    }
    wholeDuration = 1000 * 100 / tempoFactor;
    const char* label = (tempoFactor < 80) ? "SLOW" : (tempoFactor < 130) ? "NORMAL" : "FAST";
    Serial.print(F("[SPEED] "));
    Serial.print(label);
    Serial.print(F(" ("));
    Serial.print(tempoFactor);
    Serial.println(F("%)"));
  }
}

// ─── 串口命令处理 / Serial Command Handler ────────────────────
void handleSerial() {
  if (!Serial.available()) return;
  char cmd = (char)Serial.read();

  // 数字 1-5 选曲
  if (cmd >= '1' && cmd <= '5') {
    int idx = cmd - '1';
    isPlaying   = false;
    isStopped   = true;
    currentNote = 0;
    noTone(PIN_BUZZER);
    currentMelody = idx;
    Serial.print(F("[SEL] "));
    Serial.print(idx + 1);
    Serial.print(F(". "));
    Serial.println(melodyNames[currentMelody]);
    return;
  }

  switch (cmd) {
    case 'P': case 'p':
      if (isStopped) {
        isPlaying = true; isStopped = false; currentNote = 0;
        Serial.print(F("[PLAY] ")); Serial.println(melodyNames[currentMelody]);
        startNextNote();
      } else if (isPlaying) {
        isPlaying = false; noTone(PIN_BUZZER); digitalWrite(PIN_LED, LOW);
        Serial.println(F("[PAUSE]"));
      } else {
        isPlaying = true;
        Serial.println(F("[RESUME]"));
        startNextNote();
      }
      break;

    case 'S': case 's':
      isPlaying = false; isStopped = true; currentNote = 0;
      noTone(PIN_BUZZER); digitalWrite(PIN_LED, LOW);
      Serial.println(F("[STOP]"));
      break;

    case '+':
      tempoFactor = min(200, tempoFactor + 20);
      wholeDuration = 1000 * 100 / tempoFactor;
      Serial.print(F("[SPEED+] ")); Serial.print(tempoFactor); Serial.println(F("%"));
      break;

    case '-':
      tempoFactor = max(40, tempoFactor - 20);
      wholeDuration = 1000 * 100 / tempoFactor;
      Serial.print(F("[SPEED-] ")); Serial.print(tempoFactor); Serial.println(F("%"));
      break;

    case 'L': case 'l':
      Serial.println(F("=== Melody List ==="));
      for (int i = 0; i < 5; i++) {
        Serial.print(i + 1);
        Serial.print(F(". "));
        Serial.print(melodyNames[i]);
        Serial.print(F(" ("));
        Serial.print(melodyLens[i]);
        Serial.println(F(" notes)"));
      }
      Serial.print(F("Current: ")); Serial.println(currentMelody + 1);
      Serial.print(F("Tempo: ")); Serial.print(tempoFactor); Serial.println(F("%"));
      break;

    case 'H': case 'h': case '?':
      Serial.println(F("=== Commands ==="));
      Serial.println(F("1-5  Select melody"));
      Serial.println(F("P    Play / Pause / Resume"));
      Serial.println(F("S    Stop"));
      Serial.println(F("+    Speed up (+20%)"));
      Serial.println(F("-    Speed down (-20%)"));
      Serial.println(F("L    List melodies"));
      break;
  }
}

// ─── Setup ────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);

  pinMode(PIN_BUZZER,    OUTPUT);
  pinMode(PIN_LED,       OUTPUT);
  pinMode(PIN_BTN_SEL,   INPUT_PULLUP);
  pinMode(PIN_BTN_PLAY,  INPUT_PULLUP);
  pinMode(PIN_BTN_SPEED, INPUT_PULLUP);

  wholeDuration = 1000 * 100 / tempoFactor;

  Serial.println(F("=== Buzzer Melody Player ==="));
  Serial.println(F("Pin 8=Buzzer  Pin 13=LED"));
  Serial.println(F("Pin 2=Select  Pin 3=Play/Stop  Pin 4=Speed"));
  Serial.println(F("Send H for help, L to list melodies"));
  Serial.println(F("----------------------------"));

  // 开机提示音 / Startup chime
  tone(PIN_BUZZER, NOTE_C5, 100); delay(150);
  tone(PIN_BUZZER, NOTE_E5, 100); delay(150);
  tone(PIN_BUZZER, NOTE_G5, 200); delay(250);
  noTone(PIN_BUZZER);

  Serial.print(F("Ready. Melody 1: "));
  Serial.println(melodyNames[0]);
}

// ─── Loop ─────────────────────────────────────────────────────
void loop() {
  handleButtons();
  handleSerial();

  // 非阻塞音符推进 / Non-blocking note advance
  if (isPlaying && noteActive) {
    if (millis() - noteStartTime >= (unsigned long)noteDuration) {
      noteActive = false;
      noTone(PIN_BUZZER);
      digitalWrite(PIN_LED, LOW);
      startNextNote();
    }
  }
}