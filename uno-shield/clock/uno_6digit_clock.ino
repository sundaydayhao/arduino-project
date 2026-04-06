#include <Wire.h>
#include <RTClib.h>
#include <DHT.h>
#include <math.h>

// ---------- User configuration ----------
#define DIGIT_COUNT 6
#define DHTPIN A2
#define DHTTYPE DHT22   // Change to DHT11 if you use DHT11.

// false: common cathode digits + NPN low-side digit transistors
// true : common anode digits + PNP/P-MOS high-side digit transistors (18101BS default)
const bool COMMON_ANODE = true;

// Segment order: a, b, c, d, e, f, g, dp
const uint8_t segmentPins[8] = {2, 3, 4, 5, 6, 7, 8, 9};
const uint8_t digitPins[DIGIT_COUNT] = {10, 11, 12, 13, A0, A1};

const unsigned long REFRESH_INTERVAL_US = 1000;
const unsigned long PAGE_INTERVAL_MS = 3500;
const unsigned long SENSOR_INTERVAL_MS = 2000;
const unsigned long CONTENT_INTERVAL_MS = 100;

RTC_DS3231 rtc;
DHT dht(DHTPIN, DHTTYPE);

enum Page : uint8_t {
  PAGE_TIME = 0,
  PAGE_DATE = 1,
  PAGE_TEMP = 2,
  PAGE_HUMI = 3,
  PAGE_COUNT = 4
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

void showTime(const DateTime &now) {
  clearDisplayBuffer();

  bool blink = (now.second() % 2) == 0;
  setGlyph(0, '0' + now.hour() / 10);
  setGlyph(1, '0' + now.hour() % 10, blink);
  setGlyph(2, '0' + now.minute() / 10);
  setGlyph(3, '0' + now.minute() % 10, blink);
  setGlyph(4, '0' + now.second() / 10);
  setGlyph(5, '0' + now.second() % 10);
}

void showDate(const DateTime &now) {
  clearDisplayBuffer();

  uint8_t year2 = now.year() % 100;
  setGlyph(0, '0' + year2 / 10);
  setGlyph(1, '0' + year2 % 10, true);
  setGlyph(2, '0' + now.month() / 10);
  setGlyph(3, '0' + now.month() % 10, true);
  setGlyph(4, '0' + now.day() / 10);
  setGlyph(5, '0' + now.day() % 10);
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

void updateDisplayContent() {
  unsigned long nowMs = millis();
  if (nowMs - lastContentMs < CONTENT_INTERVAL_MS) {
    return;
  }
  lastContentMs = nowMs;

  Page page = static_cast<Page>((nowMs / PAGE_INTERVAL_MS) % PAGE_COUNT);

  switch (page) {
    case PAGE_TIME:
      showTime(rtc.now());
      break;
    case PAGE_DATE:
      showDate(rtc.now());
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
  if (!rtc.begin()) {
    while (true) {
      clearDisplayBuffer();
      setGlyph(0, 'r');
      setGlyph(1, 't');
      setGlyph(2, 'c');
      refreshDisplay();
    }
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // If you want to set a fixed time manually, uncomment once, upload, then comment it back:
  // rtc.adjust(DateTime(2026, 4, 6, 12, 0, 0));
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

  Wire.begin();
  dht.begin();
  setupRtc();
}

void loop() {
  refreshDisplay();
  updateSensors();
  updateDisplayContent();
}
