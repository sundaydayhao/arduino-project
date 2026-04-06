#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

// ---------- User configuration ----------
const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Default: Shanghai current weather from Open-Meteo.
// You can replace latitude / longitude / timezone with your own location.
const char *WEATHER_URL =
    "https://api.open-meteo.com/v1/forecast"
    "?latitude=31.2304"
    "&longitude=121.4737"
    "&current=temperature_2m,relative_humidity_2m"
    "&timezone=Asia%2FShanghai";

constexpr uint8_t DIGIT_COUNT = 6;
constexpr bool COMMON_ANODE = true;

// 74HCT595 control pins on NodeMCU.
constexpr uint8_t SHIFT_DATA_PIN = D7;
constexpr uint8_t SHIFT_CLOCK_PIN = D5;
constexpr uint8_t SHIFT_LATCH_PIN = D6;

constexpr unsigned long REFRESH_INTERVAL_US = 1200;
constexpr unsigned long PAGE_INTERVAL_MS = 3500;
constexpr unsigned long CONTENT_INTERVAL_MS = 100;
constexpr unsigned long WIFI_RETRY_INTERVAL_MS = 10000;
constexpr unsigned long WEATHER_INTERVAL_MS = 10UL * 60UL * 1000UL;

struct ClockTime {
  uint16_t year;
  uint8_t month;
  uint8_t day;
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

enum SegmentMask : uint8_t {
  SEG_A = 0x01,
  SEG_B = 0x02,
  SEG_C = 0x04,
  SEG_D = 0x08,
  SEG_E = 0x10,
  SEG_F = 0x20,
  SEG_G = 0x40,
  SEG_DP = 0x80
};

const uint8_t digitFont[10] = {
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,         // 0
    SEG_B | SEG_C,                                         // 1
    SEG_A | SEG_B | SEG_D | SEG_E | SEG_G,                 // 2
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_G,                 // 3
    SEG_B | SEG_C | SEG_F | SEG_G,                         // 4
    SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,                 // 5
    SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,         // 6
    SEG_A | SEG_B | SEG_C,                                 // 7
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G, // 8
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G          // 9
};

uint8_t displayBuffer[DIGIT_COUNT] = {0};
uint8_t currentDigit = 0;

unsigned long lastRefreshUs = 0;
unsigned long lastContentMs = 0;
unsigned long lastWeatherAttemptMs = 0;
unsigned long lastWiFiRetryMs = 0;

float lastTemperature = NAN;
float lastHumidity = NAN;
bool weatherEverLoaded = false;

uint8_t encodeChar(char ch) {
  if (ch >= '0' && ch <= '9') {
    return digitFont[ch - '0'];
  }

  switch (ch) {
    case ' ':
      return 0;
    case '-':
      return SEG_G;
    case 'A':
    case 'a':
      return SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G;
    case 'C':
    case 'c':
      return SEG_A | SEG_D | SEG_E | SEG_F;
    case 'E':
    case 'e':
      return SEG_A | SEG_D | SEG_E | SEG_F | SEG_G;
    case 'F':
    case 'f':
      return SEG_A | SEG_E | SEG_F | SEG_G;
    case 'H':
      return SEG_B | SEG_C | SEG_E | SEG_F | SEG_G;
    case 'h':
      return SEG_C | SEG_E | SEG_F | SEG_G;
    case 'L':
    case 'l':
      return SEG_D | SEG_E | SEG_F;
    case 'n':
      return SEG_C | SEG_E | SEG_G;
    case 'o':
      return SEG_C | SEG_D | SEG_E | SEG_G;
    case 'P':
    case 'p':
      return SEG_A | SEG_B | SEG_E | SEG_F | SEG_G;
    case 'r':
      return SEG_E | SEG_G;
    case 't':
      return SEG_D | SEG_E | SEG_F | SEG_G;
    case 'u':
      return SEG_C | SEG_D | SEG_E;
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

void writeShiftRegisters(uint8_t segmentState, uint8_t digitState) {
  digitalWrite(SHIFT_LATCH_PIN, LOW);
  shiftOut(SHIFT_DATA_PIN, SHIFT_CLOCK_PIN, MSBFIRST, digitState);
  shiftOut(SHIFT_DATA_PIN, SHIFT_CLOCK_PIN, MSBFIRST, segmentState);
  digitalWrite(SHIFT_LATCH_PIN, HIGH);
}

void blankDisplayOutput() {
  writeShiftRegisters(COMMON_ANODE ? 0xFF : 0x00, COMMON_ANODE ? 0xFF : 0x00);
}

void refreshDisplay() {
  unsigned long nowUs = micros();
  if (nowUs - lastRefreshUs < REFRESH_INTERVAL_US) {
    return;
  }
  lastRefreshUs = nowUs;

  uint8_t mask = displayBuffer[currentDigit];
  uint8_t segmentState = COMMON_ANODE ? static_cast<uint8_t>(~mask) : mask;
  uint8_t digitState = COMMON_ANODE ? 0xFF : 0x00;

  if (COMMON_ANODE) {
    digitState &= static_cast<uint8_t>(~(1U << currentDigit));
  } else {
    digitState |= static_cast<uint8_t>(1U << currentDigit);
  }

  writeShiftRegisters(segmentState, digitState);
  currentDigit = (currentDigit + 1) % DIGIT_COUNT;
}

bool getClockTime(ClockTime &clock) {
  time_t now = time(nullptr);
  if (now < 1700000000) {
    return false;
  }

  tm timeInfo;
  localtime_r(&now, &timeInfo);

  clock.year = static_cast<uint16_t>(timeInfo.tm_year + 1900);
  clock.month = static_cast<uint8_t>(timeInfo.tm_mon + 1);
  clock.day = static_cast<uint8_t>(timeInfo.tm_mday);
  clock.hour = static_cast<uint8_t>(timeInfo.tm_hour);
  clock.minute = static_cast<uint8_t>(timeInfo.tm_min);
  clock.second = static_cast<uint8_t>(timeInfo.tm_sec);
  return true;
}

Page getCurrentPage(unsigned long nowMs) {
  return static_cast<Page>((nowMs / PAGE_INTERVAL_MS) % PAGE_COUNT);
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

  uint8_t year2 = static_cast<uint8_t>(now.year % 100);
  setGlyph(0, '0' + year2 / 10);
  setGlyph(1, '0' + year2 % 10, true);
  setGlyph(2, '0' + now.month / 10);
  setGlyph(3, '0' + now.month % 10, true);
  setGlyph(4, '0' + now.day / 10);
  setGlyph(5, '0' + now.day % 10);
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

  int scaled = static_cast<int>(lroundf(value * 10.0f));
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

void showStatusPage(char first, char second) {
  clearDisplayBuffer();
  setGlyph(0, first);
  setGlyph(1, second);
  setGlyph(2, '-');
  setGlyph(3, '-');
  setGlyph(4, '-');
  setGlyph(5, ' ');
}

bool extractJsonNumber(const String &payload, const char *key, float &value) {
  String token = "\"";
  token += key;
  token += "\":";

  int searchFrom = 0;
  while (searchFrom >= 0) {
    int start = payload.indexOf(token, searchFrom);
    if (start < 0) {
      return false;
    }

    start += token.length();
    while (start < payload.length() &&
           isspace(static_cast<unsigned char>(payload[start]))) {
      ++start;
    }

    int end = start;
    while (end < payload.length()) {
      char ch = payload[end];
      if ((ch >= '0' && ch <= '9') || ch == '-' || ch == '+' || ch == '.') {
        ++end;
        continue;
      }
      break;
    }

    if (end > start) {
      value = payload.substring(start, end).toFloat();
      return true;
    }

    searchFrom = start;
  }

  return false;
}

void ensureWiFiConnected() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  unsigned long nowMs = millis();
  if (nowMs - lastWiFiRetryMs < WIFI_RETRY_INTERVAL_MS) {
    return;
  }
  lastWiFiRetryMs = nowMs;

  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

bool fetchWeatherNow() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  BearSSL::WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  if (!https.begin(client, WEATHER_URL)) {
    return false;
  }

  https.setTimeout(6000);
  int httpCode = https.GET();
  if (httpCode != HTTP_CODE_OK) {
    https.end();
    return false;
  }

  String payload = https.getString();
  https.end();

  float temp = NAN;
  float humidity = NAN;
  bool gotTemp = extractJsonNumber(payload, "temperature_2m", temp);
  bool gotHumidity = extractJsonNumber(payload, "relative_humidity_2m", humidity);

  if (gotTemp) {
    lastTemperature = temp;
  }
  if (gotHumidity) {
    lastHumidity = humidity;
  }

  weatherEverLoaded = weatherEverLoaded || gotTemp || gotHumidity;
  return gotTemp || gotHumidity;
}

void updateWeather() {
  unsigned long nowMs = millis();

  if (!weatherEverLoaded) {
    if (nowMs - lastWeatherAttemptMs < 15000) {
      return;
    }
  } else if (nowMs - lastWeatherAttemptMs < WEATHER_INTERVAL_MS) {
    return;
  }

  lastWeatherAttemptMs = nowMs;
  fetchWeatherNow();
}

void updateDisplayContent() {
  unsigned long nowMs = millis();
  if (nowMs - lastContentMs < CONTENT_INTERVAL_MS) {
    return;
  }
  lastContentMs = nowMs;

  ClockTime now;
  bool timeOk = getClockTime(now);
  Page page = getCurrentPage(nowMs);

  switch (page) {
    case PAGE_TIME:
      if (timeOk) {
        showTime(now);
      } else if (WiFi.status() == WL_CONNECTED) {
        showStatusPage('n', 't');
      } else {
        showStatusPage('n', 'o');
      }
      break;
    case PAGE_DATE:
      if (timeOk) {
        showDate(now);
      } else if (WiFi.status() == WL_CONNECTED) {
        showStatusPage('n', 't');
      } else {
        showStatusPage('n', 'o');
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

void setup() {
  pinMode(SHIFT_DATA_PIN, OUTPUT);
  pinMode(SHIFT_CLOCK_PIN, OUTPUT);
  pinMode(SHIFT_LATCH_PIN, OUTPUT);

  blankDisplayOutput();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  configTime(8 * 3600, 0, "ntp1.aliyun.com", "ntp.ntsc.ac.cn", "pool.ntp.org");
}

void loop() {
  refreshDisplay();
  ensureWiFiConnected();
  updateWeather();
  updateDisplayContent();
}
