/**
 * ============================================================
 * Arduino Clock Project
 * ============================================================
 *
 * Supported boards:
 *   - Arduino UNO R3
 *   - Arduino Mega 2560
 *   - Lion D1 R1 / LOLIN(WeMos) D1 R1
 *
 * LCD1602 parallel wiring used in this sketch:
 *   Arduino UNO R3:
 *   - RS -> D8
 *   - EN -> D9
 *   - D4 -> D4
 *   - D5 -> D5
 *   - D6 -> D6
 *   - D7 -> D7
 *
 *   Arduino Mega 2560:
 *   - RS -> D8
 *   - EN -> D9
 *   - D4 -> D4
 *   - D5 -> D5
 *   - D6 -> D6
 *   - D7 -> D7
 *
 *   Lion D1 R1 / LOLIN(WeMos) D1 R1:
 *   - RS -> D8
 *   - EN -> D9
 *   - D4 -> D4
 *   - D5 -> D5
 *   - D6 -> D6
 *   - D7 -> D7
 *
 * Notes for D1 R1:
 *   - The ESP8266 board must use D-pin macros such as D8/D9/D4.
 *   - Do not replace them with raw numbers, or they will map to GPIO IDs.
 *   - RW -> GND
 *   - VSS -> GND
 *   - VDD -> 5V
 *   - VO  -> contrast potentiometer middle pin
 *   - A / LED+ -> 5V (or 5V through resistor, depends on module)
 *   - K / LED- -> GND
 *
 * Notes:
 *   - This version does not use an RTC module.
 *   - Time is maintained in software using millis().
 *   - Time is lost after reset or power-off.
 *
 * Required Libraries:
 *   - LiquidCrystal (Arduino built-in library)
 *
 * Serial commands:
 *   - T2024-03-25 12:30:00  Set software clock
 *   - R                     Read current time
 * ============================================================
 */

#include <LiquidCrystal.h>
#include <stdlib.h>
#include <string.h>

#if defined(ARDUINO_AVR_UNO)
#define BOARD_NAME "Arduino UNO R3"
#define LCD_RS 8
#define LCD_EN 9
#define LCD_D4 4
#define LCD_D5 5
#define LCD_D6 6
#define LCD_D7 7
#elif defined(ARDUINO_AVR_MEGA2560)
#define BOARD_NAME "Arduino Mega 2560"
#define LCD_RS 8
#define LCD_EN 9
#define LCD_D4 4
#define LCD_D5 5
#define LCD_D6 6
#define LCD_D7 7
#elif defined(ARDUINO_ESP8266_WEMOS_D1R1)
#define BOARD_NAME "Lion D1 R1 / LOLIN(WeMos) D1 R1"
#define LCD_RS D8
#define LCD_EN D9
#define LCD_D4 D4
#define LCD_D5 D5
#define LCD_D6 D6
#define LCD_D7 D7
#else
#error "Unsupported board. Select Arduino UNO R3, Arduino Mega 2560, or LOLIN(WeMos) D1 R1 / Lion D1 R1."
#endif

#define LCD_COLS 16
#define LCD_ROWS 2
#define SERIAL_BAUD 9600

struct ClockDateTime {
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
};

LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

const char* dayNames[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

ClockDateTime currentTime;
unsigned long lastTickMillis = 0;
bool needsDisplayRefresh = true;

bool isLeapYear(int year) {
  if (year % 400 == 0) {
    return true;
  }
  if (year % 100 == 0) {
    return false;
  }
  return (year % 4 == 0);
}

int daysInMonth(int year, int month) {
  static const int monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  if (month == 2 && isLeapYear(year)) {
    return 29;
  }

  return monthDays[month - 1];
}

bool isValidDateTime(const ClockDateTime& dt) {
  if (dt.year < 2000 || dt.year > 2099) {
    return false;
  }
  if (dt.month < 1 || dt.month > 12) {
    return false;
  }
  if (dt.day < 1 || dt.day > daysInMonth(dt.year, dt.month)) {
    return false;
  }
  if (dt.hour < 0 || dt.hour > 23) {
    return false;
  }
  if (dt.minute < 0 || dt.minute > 59) {
    return false;
  }
  if (dt.second < 0 || dt.second > 59) {
    return false;
  }
  return true;
}

int monthFromAbbrev(const char* monthStr) {
  const char* months[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };

  for (int i = 0; i < 12; ++i) {
    if (strncmp(monthStr, months[i], 3) == 0) {
      return i + 1;
    }
  }

  return 1;
}

void loadCompileDateTime() {
  char monthStr[4];
  monthStr[0] = __DATE__[0];
  monthStr[1] = __DATE__[1];
  monthStr[2] = __DATE__[2];
  monthStr[3] = '\0';

  currentTime.year = atoi(__DATE__ + 7);
  currentTime.month = monthFromAbbrev(monthStr);
  currentTime.day = atoi(__DATE__ + 4);
  currentTime.hour = atoi(__TIME__);
  currentTime.minute = atoi(__TIME__ + 3);
  currentTime.second = atoi(__TIME__ + 6);
}

int dayOfWeek(int year, int month, int day) {
  static const int offsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};

  if (month < 3) {
    year -= 1;
  }

  return (year + year / 4 - year / 100 + year / 400 + offsets[month - 1] + day) % 7;
}

void incrementOneSecond() {
  currentTime.second++;

  if (currentTime.second < 60) {
    return;
  }
  currentTime.second = 0;
  currentTime.minute++;

  if (currentTime.minute < 60) {
    return;
  }
  currentTime.minute = 0;
  currentTime.hour++;

  if (currentTime.hour < 24) {
    return;
  }
  currentTime.hour = 0;
  currentTime.day++;

  if (currentTime.day <= daysInMonth(currentTime.year, currentTime.month)) {
    return;
  }
  currentTime.day = 1;
  currentTime.month++;

  if (currentTime.month <= 12) {
    return;
  }
  currentTime.month = 1;
  currentTime.year++;
}

void updateSoftwareClock() {
  unsigned long nowMillis = millis();

  while ((unsigned long)(nowMillis - lastTickMillis) >= 1000UL) {
    lastTickMillis += 1000UL;
    incrementOneSecond();
    needsDisplayRefresh = true;
  }
}

void printPaddedLine(const char* text) {
  lcd.print(text);

  int len = strlen(text);
  for (int i = len; i < LCD_COLS; ++i) {
    lcd.print(' ');
  }
}

void displayClock() {
  if (!needsDisplayRefresh) {
    return;
  }

  char line1[17];
  char line2[17];

  snprintf(
    line1,
    sizeof(line1),
    "%04d-%02d-%02d %s",
    currentTime.year,
    currentTime.month,
    currentTime.day,
    dayNames[dayOfWeek(currentTime.year, currentTime.month, currentTime.day)]
  );

  snprintf(
    line2,
    sizeof(line2),
    "%02d:%02d:%02d No RTC",
    currentTime.hour,
    currentTime.minute,
    currentTime.second
  );

  lcd.setCursor(0, 0);
  printPaddedLine(line1);
  lcd.setCursor(0, 1);
  printPaddedLine(line2);

  Serial.print(F("[TIME] "));
  Serial.print(line1);
  Serial.print(F("  "));
  Serial.println(line2);

  needsDisplayRefresh = false;
}

void showTimeUpdatedMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Time Updated!   "));
  lcd.setCursor(0, 1);
  lcd.print(F("Software Clock  "));
  delay(1500);
  lcd.clear();
}

void printCurrentTimeToSerial() {
  char buf[40];

  snprintf(
    buf,
    sizeof(buf),
    "[CLOCK] %04d-%02d-%02d %02d:%02d:%02d %s",
    currentTime.year,
    currentTime.month,
    currentTime.day,
    currentTime.hour,
    currentTime.minute,
    currentTime.second,
    dayNames[dayOfWeek(currentTime.year, currentTime.month, currentTime.day)]
  );

  Serial.println(buf);
}

void handleSerial() {
  if (!Serial.available()) {
    return;
  }

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd.length() == 0) {
    return;
  }

  if (cmd == "R" || cmd == "r") {
    printCurrentTimeToSerial();
    return;
  }

  if (cmd.charAt(0) == 'T' || cmd.charAt(0) == 't') {
    if (cmd.length() < 20) {
      Serial.println(F("[ERROR] Format: T2024-03-25 12:30:00"));
      return;
    }

    ClockDateTime newTime;
    newTime.year = cmd.substring(1, 5).toInt();
    newTime.month = cmd.substring(6, 8).toInt();
    newTime.day = cmd.substring(9, 11).toInt();
    newTime.hour = cmd.substring(12, 14).toInt();
    newTime.minute = cmd.substring(15, 17).toInt();
    newTime.second = cmd.substring(18, 20).toInt();

    if (!isValidDateTime(newTime)) {
      Serial.println(F("[ERROR] Invalid date/time values."));
      return;
    }

    currentTime = newTime;
    lastTickMillis = millis();
    needsDisplayRefresh = true;

    Serial.print(F("[OK] Time set to: "));
    printCurrentTimeToSerial();
    showTimeUpdatedMessage();
    return;
  }

  Serial.println(F("[HELP] Commands:"));
  Serial.println(F("  R                     - Read current time"));
  Serial.println(F("  T2024-03-25 12:30:00  - Set time"));
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println(F("============================="));
  Serial.print(F("  "));
  Serial.print(F(BOARD_NAME));
  Serial.println(F(" Clock"));
  Serial.println(F("============================="));
  Serial.print(F("[BOARD] "));
  Serial.println(F(BOARD_NAME));

  lcd.begin(LCD_COLS, LCD_ROWS);
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(F(" Arduino  Clock "));
  lcd.setCursor(0, 1);
  lcd.print(F(" Software Clock "));
  delay(1500);

  loadCompileDateTime();
  lastTickMillis = millis();
  needsDisplayRefresh = true;

  Serial.println(F("[OK] Software clock initialized."));
  Serial.println(F("[TIP] No RTC hardware is used in this version."));
  Serial.println(F("[TIP] Time starts from compile time after reset."));
  Serial.println(F("[TIP] Send 'T2024-03-25 12:30:00' to set time."));
  Serial.println(F("[TIP] Send 'R' to read current time."));
}

void loop() {
  handleSerial();
  updateSoftwareClock();
  displayClock();
  delay(200);
}
