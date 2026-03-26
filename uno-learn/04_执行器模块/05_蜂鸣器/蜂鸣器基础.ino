/*
 * 实验名称：蜂鸣器控制
 * 实验目的：学习有源和无源蜂鸣器的使用，播放音乐
 * 
 * 硬件连接：
 *   蜂鸣器正极 → D8
 *   蜂鸣器负极 → GND
 * 
 * 有源蜂鸣器：内置振荡器，通电即响，只能发出固定频率
 * 无源蜂鸣器：需要外部 PWM 信号驱动，可以发出不同音调
 * 
 * 知识点：
 *   - tone(pin, frequency, duration) 产生方波
 *   - noTone(pin) 停止发声
 *   - 音符频率对应关系
 */

const int BUZZER_PIN = 8;

// 音符频率定义（Hz）
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define REST     0

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.begin(9600);
  Serial.println("蜂鸣器实验");
  Serial.println("输入命令: beep/scale/mario/birthday");
}

void loop() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();
    
    if (cmd == "beep") {
      beep(1000, 500);
      Serial.println("哔！");
    }
    else if (cmd == "scale") {
      playScale();
    }
    else if (cmd == "mario") {
      playMario();
    }
    else if (cmd == "birthday") {
      playHappyBirthday();
    }
    else {
      Serial.println("命令: beep/scale/mario/birthday");
    }
  }
}

// 发出一声蜂鸣
void beep(int frequency, int duration) {
  tone(BUZZER_PIN, frequency, duration);
  delay(duration + 50);
  noTone(BUZZER_PIN);
}

// 播放音阶
void playScale() {
  Serial.println("播放音阶...");
  int notes[] = {NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4, 
                 NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5};
  for (int i = 0; i < 8; i++) {
    tone(BUZZER_PIN, notes[i], 300);
    delay(350);
  }
  noTone(BUZZER_PIN);
}

// 播放超级马里奥主题曲（片段）
void playMario() {
  Serial.println("播放马里奥...");
  int melody[] = {
    NOTE_E5, NOTE_E5, REST, NOTE_E5, REST, NOTE_C5, NOTE_E5, REST,
    NOTE_G5, REST, REST, REST, NOTE_G4, REST, REST, REST
  };
  int durations[] = {
    150, 150, 150, 150, 150, 150, 150, 150,
    150, 150, 150, 150, 150, 150, 150, 150
  };
  
  for (int i = 0; i < 16; i++) {
    if (melody[i] == REST) {
      delay(durations[i]);
    } else {
      tone(BUZZER_PIN, melody[i], durations[i]);
      delay(durations[i] * 1.3);
    }
    noTone(BUZZER_PIN);
  }
}

// 播放生日快乐
void playHappyBirthday() {
  Serial.println("播放生日快乐...");
  int melody[] = {
    NOTE_C4, NOTE_C4, NOTE_D4, NOTE_C4, NOTE_F4, NOTE_E4,
    NOTE_C4, NOTE_C4, NOTE_D4, NOTE_C4, NOTE_G4, NOTE_F4,
    NOTE_C4, NOTE_C4, NOTE_C5, NOTE_A4, NOTE_F4, NOTE_E4, NOTE_D4,
    NOTE_A4, NOTE_A4, NOTE_A4, NOTE_F4, NOTE_G4, NOTE_F4
  };
  int durations[] = {
    300, 300, 600, 600, 600, 1200,
    300, 300, 600, 600, 600, 1200,
    300, 300, 600, 600, 600, 600, 1200,
    300, 300, 600, 600, 600, 1200
  };
  
  for (int i = 0; i < 25; i++) {
    tone(BUZZER_PIN, melody[i], durations[i]);
    delay(durations[i] * 1.3);
    noTone(BUZZER_PIN);
  }
}