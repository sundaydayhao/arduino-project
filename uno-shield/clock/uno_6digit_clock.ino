#include <DHT.h>
#include <math.h>

// ---------- User configuration ----------
#define DIGIT_COUNT 6
#define DHTPIN A2
#define DHTTYPE DHT22   // Change to DHT11 if you use DHT11.
#define RTC_CE_PIN A3
#define RTC_IO_PIN A4
#define RTC_SCLK_PIN A5
#define BUTTON_ADC_PIN A0

// false: common cathode digits + NPN low-side digit transistors
// true : common anode digits + PNP/P-MOS high-side digit transistors (18101BS default)
const bool COMMON_ANODE = true;

// Segment order: a, b, c, d, e, f, g, dp
const uint8_t segmentPins[8] = {2, 3, 4, 5, 6, 7, 8, 9};
// D1(TX) is reused as the 5th digit select so A0 can be reserved for the 4-key resistor ladder.
const uint8_t digitPins[DIGIT_COUNT] = {10, 11, 12, 13, 1, A1};

const unsigned long REFRESH_INTERVAL_US = 1000;
const unsigned long PAGE_INTERVAL_MS = 3500;
const unsigned long SENSOR_INTERVAL_MS = 2000;
const unsigned long CONTENT_INTERVAL_MS = 100;
const unsigned long BUTTON_DEBOUNCE_MS = 30;
const unsigned long BUTTON_LONG_PRESS_MS = 800;
const unsigned long BUTTON_REPEAT_START_MS = 500;
const unsigned long BUTTON_REPEAT_INTERVAL_MS = 180;
const unsigned long MANUAL_PAGE_HOLD_MS = 10000;

DHT dht(DHTPIN, DHTTYPE);

struct ClockTime {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t dayOfWeek;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};

enum Page : uint8_t {
  PAGE_TIME = 0,
  PAGE_DATE = 1,
  PAGE_TEMP = 2,
  PAGE_HUMI = 3,
  PAGE_COUNT = 4
};

enum ButtonId : uint8_t {
  BUTTON_NONE = 0,
  BUTTON_FUNCTION = 1,
  BUTTON_SELECT = 2,
  BUTTON_UP = 3,
  BUTTON_DOWN = 4
};

enum UiMode : uint8_t {
  UI_VIEW = 0,
  UI_EDIT = 1
};

enum EditField : uint8_t {
  EDIT_HOUR = 0,
  EDIT_MINUTE = 1,
  EDIT_SECOND = 2,
  EDIT_YEAR = 3,
  EDIT_MONTH = 4,
  EDIT_DAY = 5,
  EDIT_COUNT = 6
};

enum SegmentMask : uint8_t {
  SEG_A  = 0x01,
  SEG_B  = 0x02,
  SEG_C  = 0x04,
  SEG_D  = 0x08,
  SEG_E  = 0x10,
  SEG_F  = 0x20,
  SEG_G  = 0x40,
  SEG_DP = 0x80
};

const uint8_t digitFont[10] = {
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,             // 0
  SEG_B | SEG_C,                                             // 1
  SEG_A | SEG_B | SEG_D | SEG_E | SEG_G,                     // 2
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_G,                     // 3
  SEG_B | SEG_C | SEG_F | SEG_G,                             // 4
  SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,                     // 5
  SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,             // 6
  SEG_A | SEG_B | SEG_C,                                     // 7
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,     // 8
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G              // 9
};

uint8_t displayBuffer[DIGIT_COUNT] = {0};
uint8_t currentDigit = 0;

unsigned long lastRefreshUs = 0;
unsigned long lastSensorMs = 0;
unsigned long lastContentMs = 0;

float lastTemperature = NAN;
float lastHumidity = NAN;

UiMode uiMode = UI_VIEW;
Page manualPage = PAGE_TIME;
unsigned long manualPageUntilMs = 0;
ClockTime editTime = {2026, 1, 1, 1, 0, 0, 0};
EditField editField = EDIT_HOUR;

ButtonId lastRawButton = BUTTON_NONE;
ButtonId stableButton = BUTTON_NONE;
ButtonId eventButton = BUTTON_NONE;
bool eventIsLong = false;
bool eventPending = false;
bool pressConsumed = false;
unsigned long lastButtonChangeMs = 0;
unsigned long stablePressStartMs = 0;
unsigned long lastRepeatMs = 0;

const uint8_t DS1302_REG_WP = 0x8E;
const uint8_t DS1302_CMD_CLOCK_BURST_READ = 0xBF;
const uint8_t DS1302_CMD_CLOCK_BURST_WRITE = 0xBE;

uint8_t bcdToDec(uint8_t value) {
  return ((value >> 4) * 10) + (value & 0x0F);
}

uint8_t decToBcd(uint8_t value) {
  return ((value / 10) << 4) | (value % 10);
}

bool isLeapYear(uint16_t year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

uint8_t daysInMonth(uint16_t year, uint8_t month) {
  static const uint8_t daysPerMonth[12] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
  };

  if (month == 2 && isLeapYear(year)) {
    return 29;
  }

  if (month < 1 || month > 12) {
    return 31;
  }

  return daysPerMonth[month - 1];
}

uint8_t calculateDayOfWeek(uint16_t year, uint8_t month, uint8_t day) {
  // Return 1..7 for Sunday..Saturday to match DS1302 convention.
  static const uint8_t monthTable[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  uint16_t y = year;
  if (month < 3) {
    --y;
  }
  uint8_t dow = (y + y / 4 - y / 100 + y / 400 + monthTable[month - 1] + day) % 7;
  return dow + 1;
}

bool isValidClockTime(const ClockTime &time) {
  if (time.year < 2000 || time.year > 2099) {
    return false;
  }
  if (time.month < 1 || time.month > 12) {
    return false;
  }
  if (time.day < 1 || time.day > daysInMonth(time.year, time.month)) {
    return false;
  }
  if (time.hour > 23 || time.minute > 59 || time.second > 59) {
    return false;
  }
  return true;
}

uint8_t compileMonthFromText(const char *monthText) {
  if (monthText[0] == 'J' && monthText[1] == 'a' && monthText[2] == 'n') return 1;
  if (monthText[0] == 'F') return 2;
  if (monthText[0] == 'M' && monthText[2] == 'r') return 3;
  if (monthText[0] == 'A' && monthText[1] == 'p') return 4;
  if (monthText[0] == 'M' && monthText[2] == 'y') return 5;
  if (monthText[0] == 'J' && monthText[2] == 'n') return 6;
  if (monthText[0] == 'J' && monthText[2] == 'l') return 7;
  if (monthText[0] == 'A' && monthText[1] == 'u') return 8;
  if (monthText[0] == 'S') return 9;
  if (monthText[0] == 'O') return 10;
  if (monthText[0] == 'N') return 11;
  return 12;
}

ClockTime compileTimeToClockTime() {
  ClockTime time;
  const char *dateText = __DATE__;
  const char *timeText = __TIME__;

  time.year = (dateText[7] - '0') * 1000 + (dateText[8] - '0') * 100 +
              (dateText[9] - '0') * 10 + (dateText[10] - '0');
  time.month = compileMonthFromText(dateText);
  time.day = (dateText[4] == ' ' ? 0 : (dateText[4] - '0') * 10) + (dateText[5] - '0');
  time.hour = (timeText[0] - '0') * 10 + (timeText[1] - '0');
  time.minute = (timeText[3] - '0') * 10 + (timeText[4] - '0');
  time.second = (timeText[6] - '0') * 10 + (timeText[7] - '0');
  time.dayOfWeek = calculateDayOfWeek(time.year, time.month, time.day);

  return time;
}

void ds1302WriteByte(uint8_t value) {
  pinMode(RTC_IO_PIN, OUTPUT);
  for (uint8_t i = 0; i < 8; ++i) {
    digitalWrite(RTC_IO_PIN, value & 0x01);
    digitalWrite(RTC_SCLK_PIN, HIGH);
    digitalWrite(RTC_SCLK_PIN, LOW);
    value >>= 1;
  }
}

uint8_t ds1302ReadByte() {
  uint8_t value = 0;
  pinMode(RTC_IO_PIN, INPUT);
  for (uint8_t i = 0; i < 8; ++i) {
    if (digitalRead(RTC_IO_PIN)) {
      value |= (1 << i);
    }
    digitalWrite(RTC_SCLK_PIN, HIGH);
    digitalWrite(RTC_SCLK_PIN, LOW);
  }
  return value;
}

void ds1302BeginTransaction() {
  digitalWrite(RTC_CE_PIN, LOW);
  digitalWrite(RTC_SCLK_PIN, LOW);
  digitalWrite(RTC_CE_PIN, HIGH);
}

void ds1302EndTransaction() {
  digitalWrite(RTC_CE_PIN, LOW);
}

void ds1302WriteRegister(uint8_t reg, uint8_t value) {
  ds1302BeginTransaction();
  ds1302WriteByte(reg & 0xFE);
  ds1302WriteByte(value);
  ds1302EndTransaction();
}

void ds1302ReadBurst(uint8_t *buffer, uint8_t len) {
  ds1302BeginTransaction();
  ds1302WriteByte(DS1302_CMD_CLOCK_BURST_READ);
  for (uint8_t i = 0; i < len; ++i) {
    buffer[i] = ds1302ReadByte();
  }
  ds1302EndTransaction();
}

void ds1302WriteBurst(const uint8_t *buffer, uint8_t len) {
  ds1302BeginTransaction();
  ds1302WriteByte(DS1302_CMD_CLOCK_BURST_WRITE);
  for (uint8_t i = 0; i < len; ++i) {
    ds1302WriteByte(buffer[i]);
  }
  ds1302EndTransaction();
}

void ds1302SetWriteProtect(bool enabled) {
  ds1302WriteRegister(DS1302_REG_WP, enabled ? 0x80 : 0x00);
}

bool readRtc(ClockTime &time, bool &clockHalted) {
  uint8_t burst[8] = {0};
  ds1302ReadBurst(burst, 8);

  clockHalted = (burst[0] & 0x80) != 0;
  time.second = bcdToDec(burst[0] & 0x7F);
  time.minute = bcdToDec(burst[1] & 0x7F);

  if (burst[2] & 0x80) {
    uint8_t hour12 = bcdToDec(burst[2] & 0x1F);
    bool pm = (burst[2] & 0x20) != 0;
    time.hour = hour12 % 12;
    if (pm) {
      time.hour += 12;
    }
  } else {
    time.hour = bcdToDec(burst[2] & 0x3F);
  }

  time.day = bcdToDec(burst[3] & 0x3F);
  time.month = bcdToDec(burst[4] & 0x1F);
  time.dayOfWeek = bcdToDec(burst[5] & 0x07);
  time.year = 2000 + bcdToDec(burst[6]);

  return isValidClockTime(time);
}

void writeRtc(const ClockTime &time) {
  uint8_t burst[8];
  burst[0] = decToBcd(time.second) & 0x7F;
  burst[1] = decToBcd(time.minute);
  burst[2] = decToBcd(time.hour);   // 24-hour mode
  burst[3] = decToBcd(time.day);
  burst[4] = decToBcd(time.month);
  burst[5] = decToBcd(time.dayOfWeek);
  burst[6] = decToBcd(time.year % 100);
  burst[7] = 0x00;

  ds1302SetWriteProtect(false);
  ds1302WriteBurst(burst, 8);
  ds1302SetWriteProtect(true);
}

int wrapValue(int value, int minValue, int maxValue) {
  int span = maxValue - minValue + 1;
  while (value < minValue) {
    value += span;
  }
  while (value > maxValue) {
    value -= span;
  }
  return value;
}

void normalizeEditDate() {
  if (editTime.month < 1) {
    editTime.month = 1;
  }
  if (editTime.month > 12) {
    editTime.month = 12;
  }

  uint8_t maxDay = daysInMonth(editTime.year, editTime.month);
  if (editTime.day < 1) {
    editTime.day = 1;
  }
  if (editTime.day > maxDay) {
    editTime.day = maxDay;
  }

  editTime.dayOfWeek = calculateDayOfWeek(editTime.year, editTime.month, editTime.day);
}

Page getCurrentPage(unsigned long nowMs) {
  if (uiMode == UI_EDIT) {
    return editField <= EDIT_SECOND ? PAGE_TIME : PAGE_DATE;
  }

  if (manualPageUntilMs > nowMs) {
    return manualPage;
  }

  return static_cast<Page>((nowMs / PAGE_INTERVAL_MS) % PAGE_COUNT);
}

void holdManualPage(Page page) {
  manualPage = page;
  manualPageUntilMs = millis() + MANUAL_PAGE_HOLD_MS;
}

void startEditMode(const ClockTime &currentTime) {
  editTime = currentTime;
  normalizeEditDate();
  editField = EDIT_HOUR;
  uiMode = UI_EDIT;
}

void saveEditTime() {
  normalizeEditDate();
  writeRtc(editTime);
  uiMode = UI_VIEW;
  holdManualPage(PAGE_TIME);
}

void cancelEditMode() {
  uiMode = UI_VIEW;
  holdManualPage(PAGE_TIME);
}

void advanceEditField() {
  editField = static_cast<EditField>((editField + 1) % EDIT_COUNT);
}

void adjustEditField(int delta) {
  switch (editField) {
    case EDIT_HOUR:
      editTime.hour = wrapValue(editTime.hour + delta, 0, 23);
      break;
    case EDIT_MINUTE:
      editTime.minute = wrapValue(editTime.minute + delta, 0, 59);
      break;
    case EDIT_SECOND:
      editTime.second = wrapValue(editTime.second + delta, 0, 59);
      break;
    case EDIT_YEAR:
      editTime.year = wrapValue(editTime.year + delta, 2000, 2099);
      normalizeEditDate();
      break;
    case EDIT_MONTH:
      editTime.month = wrapValue(editTime.month + delta, 1, 12);
      normalizeEditDate();
      break;
    case EDIT_DAY:
      editTime.day = wrapValue(editTime.day + delta, 1, daysInMonth(editTime.year, editTime.month));
      normalizeEditDate();
      break;
    default:
      break;
  }
}

ButtonId decodeButtonFromAdc(int adcValue) {
  // Thresholds match a 10k pull-up on A0 and buttons to GND via 0R / 1k / 3.3k / 10k.
  if (adcValue < 50) {
    return BUTTON_FUNCTION;
  }
  if (adcValue < 170) {
    return BUTTON_SELECT;
  }
  if (adcValue < 380) {
    return BUTTON_UP;
  }
  if (adcValue < 700) {
    return BUTTON_DOWN;
  }
  return BUTTON_NONE;
}

ButtonId readButtonRaw() {
  return decodeButtonFromAdc(analogRead(BUTTON_ADC_PIN));
}

void queueButtonEvent(ButtonId button, bool isLong) {
  if (eventPending) {
    return;
  }
  eventButton = button;
  eventIsLong = isLong;
  eventPending = true;
}

uint8_t encodeChar(char ch) {
  if (ch >= '0' && ch <= '9') {
    return digitFont[ch - '0'];
  }

  switch (ch) {
    case ' ':
      return 0;
    case '-':
      return SEG_G;
    case 'C':
    case 'c':
      return SEG_A | SEG_D | SEG_E | SEG_F;
    case 'H':
      return SEG_B | SEG_C | SEG_E | SEG_F | SEG_G;
    case 'h':
      return SEG_C | SEG_E | SEG_F | SEG_G;
    case 't':
      return SEG_D | SEG_E | SEG_F | SEG_G;
    case 'r':
      return SEG_E | SEG_G;
    default:
      return 0;
  }
}

void clearDisplayBuffer() {
  for (uint8_t i = 0; i < DIGIT_COUNT; ++i) {
    displayBuffer[i] = 0;
  }
}

void setGlyph(uint8_t index, char ch, bool withDp = false) {
  if (index >= DIGIT_COUNT) {
    return;
  }

  uint8_t mask = encodeChar(ch);
  if (withDp) {
    mask |= SEG_DP;
  }
  displayBuffer[index] = mask;
}

void writeSegments(uint8_t mask) {
  for (uint8_t i = 0; i < 8; ++i) {
    bool on = mask & (1 << i);
    digitalWrite(segmentPins[i], COMMON_ANODE ? !on : on);
  }
}

void enableDigit(uint8_t index, bool enabled) {
  if (index >= DIGIT_COUNT) {
    return;
  }
  digitalWrite(digitPins[index], COMMON_ANODE ? !enabled : enabled);
}

void disableAllDigits() {
  for (uint8_t i = 0; i < DIGIT_COUNT; ++i) {
    enableDigit(i, false);
  }
}

void refreshDisplay() {
  unsigned long nowUs = micros();
  if (nowUs - lastRefreshUs < REFRESH_INTERVAL_US) {
    return;
  }
  lastRefreshUs = nowUs;

  disableAllDigits();
  writeSegments(displayBuffer[currentDigit]);
  enableDigit(currentDigit, true);
  currentDigit = (currentDigit + 1) % DIGIT_COUNT;
}

void showTime(const ClockTime &now) {
  clearDisplayBuffer();

  bool blink = (now.second % 2) == 0;
  setGlyph(0, '0' + now.hour / 10);
  setGlyph(1, '0' + now.hour % 10, blink);
  setGlyph(2, '0' + now.minute / 10);
  setGlyph(3, '0' + now.minute % 10, blink);
  setGlyph(4, '0' + now.second / 10);
  setGlyph(5, '0' + now.second % 10);
}

void showDate(const ClockTime &now) {
  clearDisplayBuffer();

  uint8_t year2 = now.year % 100;
  setGlyph(0, '0' + year2 / 10);
  setGlyph(1, '0' + year2 % 10, true);
  setGlyph(2, '0' + now.month / 10);
  setGlyph(3, '0' + now.month % 10, true);
  setGlyph(4, '0' + now.day / 10);
  setGlyph(5, '0' + now.day % 10);
}

void blankDigits(uint8_t firstIndex, uint8_t count) {
  for (uint8_t i = 0; i < count; ++i) {
    if (firstIndex + i < DIGIT_COUNT) {
      displayBuffer[firstIndex + i] = 0;
    }
  }
}

void showEditPage() {
  bool blinkVisible = ((millis() / 300) % 2) == 0;

  if (editField <= EDIT_SECOND) {
    showTime(editTime);
    if (!blinkVisible) {
      if (editField == EDIT_HOUR) {
        blankDigits(0, 2);
      } else if (editField == EDIT_MINUTE) {
        blankDigits(2, 2);
      } else {
        blankDigits(4, 2);
      }
    }
  } else {
    showDate(editTime);
    if (!blinkVisible) {
      if (editField == EDIT_YEAR) {
        blankDigits(0, 2);
      } else if (editField == EDIT_MONTH) {
        blankDigits(2, 2);
      } else {
        blankDigits(4, 2);
      }
    }
  }
}

void showFallbackPage(char prefix) {
  clearDisplayBuffer();
  setGlyph(0, prefix);
  setGlyph(1, '-');
  setGlyph(2, '-');
  setGlyph(3, '-');
  setGlyph(4, '-');
  setGlyph(5, ' ');
}

void showCompactFloat(char prefix, float value, char suffix) {
  if (isnan(value)) {
    showFallbackPage(prefix);
    return;
  }

  clearDisplayBuffer();

  if (value > 99.9f) {
    value = 99.9f;
  }
  if (value < -9.9f) {
    value = -9.9f;
  }

  int scaled = (int)lround(value * 10.0f);
  bool negative = scaled < 0;
  if (negative) {
    scaled = -scaled;
  }

  int whole = scaled / 10;
  int frac = scaled % 10;

  setGlyph(0, prefix);

  if (negative) {
    setGlyph(1, '-');
    setGlyph(2, '0' + whole, true);
    setGlyph(3, '0' + frac);
  } else if (whole >= 10) {
    setGlyph(1, '0' + (whole / 10));
    setGlyph(2, '0' + (whole % 10), true);
    setGlyph(3, '0' + frac);
  } else {
    setGlyph(1, ' ');
    setGlyph(2, '0' + whole, true);
    setGlyph(3, '0' + frac);
  }

  setGlyph(4, suffix);
  setGlyph(5, ' ');
}

void updateSensors() {
  unsigned long nowMs = millis();
  if (nowMs - lastSensorMs < SENSOR_INTERVAL_MS) {
    return;
  }
  lastSensorMs = nowMs;

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (!isnan(h)) {
    lastHumidity = h;
  }
  if (!isnan(t)) {
    lastTemperature = t;
  }
}

void updateButtons() {
  unsigned long nowMs = millis();
  ButtonId rawButton = readButtonRaw();

  if (rawButton != lastRawButton) {
    lastRawButton = rawButton;
    lastButtonChangeMs = nowMs;
  }

  if (stableButton != lastRawButton && nowMs - lastButtonChangeMs >= BUTTON_DEBOUNCE_MS) {
    ButtonId previousButton = stableButton;
    stableButton = lastRawButton;

    if (stableButton != BUTTON_NONE) {
      stablePressStartMs = nowMs;
      lastRepeatMs = nowMs;
      pressConsumed = false;
    } else if (previousButton != BUTTON_NONE && !pressConsumed) {
      queueButtonEvent(previousButton, false);
    }
  }

  if (stableButton == BUTTON_NONE) {
    return;
  }

  if (!pressConsumed && nowMs - stablePressStartMs >= BUTTON_LONG_PRESS_MS) {
    queueButtonEvent(stableButton, true);
    pressConsumed = true;
    lastRepeatMs = nowMs;
    return;
  }

  if (uiMode == UI_EDIT &&
      (stableButton == BUTTON_UP || stableButton == BUTTON_DOWN) &&
      nowMs - stablePressStartMs >= BUTTON_REPEAT_START_MS &&
      nowMs - lastRepeatMs >= BUTTON_REPEAT_INTERVAL_MS) {
    queueButtonEvent(stableButton, false);
    pressConsumed = true;
    lastRepeatMs = nowMs;
  }
}

void handleButtonEvent() {
  if (!eventPending) {
    return;
  }

  ButtonId button = eventButton;
  bool isLong = eventIsLong;
  eventPending = false;
  eventButton = BUTTON_NONE;
  eventIsLong = false;

  ClockTime now;
  bool clockHalted = false;
  bool rtcOk = readRtc(now, clockHalted) && !clockHalted;

  if (uiMode == UI_EDIT) {
    if (button == BUTTON_FUNCTION) {
      if (isLong) {
        cancelEditMode();
      } else {
        saveEditTime();
      }
      return;
    }

    if (button == BUTTON_SELECT && !isLong) {
      advanceEditField();
      return;
    }

    if (button == BUTTON_UP && !isLong) {
      adjustEditField(1);
      return;
    }

    if (button == BUTTON_DOWN && !isLong) {
      adjustEditField(-1);
      return;
    }

    return;
  }

  Page page = getCurrentPage(millis());

  if (button == BUTTON_FUNCTION && !isLong) {
    holdManualPage(static_cast<Page>((page + 1) % PAGE_COUNT));
    return;
  }

  if (button == BUTTON_UP && !isLong) {
    holdManualPage(static_cast<Page>((page + PAGE_COUNT - 1) % PAGE_COUNT));
    return;
  }

  if (button == BUTTON_DOWN && !isLong) {
    holdManualPage(static_cast<Page>((page + 1) % PAGE_COUNT));
    return;
  }

  if (button == BUTTON_SELECT && isLong) {
    if (rtcOk) {
      startEditMode(now);
    }
    return;
  }

  if (button == BUTTON_SELECT && !isLong) {
    holdManualPage(page);
  }
}

void updateDisplayContent() {
  unsigned long nowMs = millis();
  if (nowMs - lastContentMs < CONTENT_INTERVAL_MS) {
    return;
  }
  lastContentMs = nowMs;

  Page page = getCurrentPage(nowMs);
  ClockTime now;
  bool clockHalted = false;
  bool rtcOk = readRtc(now, clockHalted) && !clockHalted;

  if (uiMode == UI_EDIT) {
    showEditPage();
    return;
  }

  switch (page) {
    case PAGE_TIME:
      if (rtcOk) {
        showTime(now);
      } else {
        showFallbackPage('r');
      }
      break;
    case PAGE_DATE:
      if (rtcOk) {
        showDate(now);
      } else {
        showFallbackPage('r');
      }
      break;
    case PAGE_TEMP:
      showCompactFloat('t', lastTemperature, 'C');
      break;
    case PAGE_HUMI:
      showCompactFloat('h', lastHumidity, 'H');
      break;
    default:
      clearDisplayBuffer();
      break;
  }
}

void setupRtc() {
  pinMode(RTC_CE_PIN, OUTPUT);
  pinMode(RTC_SCLK_PIN, OUTPUT);
  pinMode(RTC_IO_PIN, INPUT);
  digitalWrite(RTC_CE_PIN, LOW);
  digitalWrite(RTC_SCLK_PIN, LOW);

  ClockTime now;
  bool clockHalted = false;
  if (!readRtc(now, clockHalted) || clockHalted) {
    writeRtc(compileTimeToClockTime());
  }

  // If you want to set a fixed time manually, replace the write below, upload once, then restore:
  // ClockTime fixed = {2026, 4, 6, 1, 12, 0, 0};
  // writeRtc(fixed);
}

void setup() {
  for (uint8_t i = 0; i < 8; ++i) {
    pinMode(segmentPins[i], OUTPUT);
  }

  for (uint8_t i = 0; i < DIGIT_COUNT; ++i) {
    pinMode(digitPins[i], OUTPUT);
  }

  disableAllDigits();
  writeSegments(0);
  pinMode(BUTTON_ADC_PIN, INPUT);

  dht.begin();
  setupRtc();
}

void loop() {
  refreshDisplay();
  updateButtons();
  handleButtonEvent();
  updateSensors();
  updateDisplayContent();
}
