#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <time.h>

// ---------- User configuration ----------
const char *WIFI_SSID = "haoge";
const char *WIFI_PASSWORD = "19901118";

// Default: Shanghai current weather from Open-Meteo.
// You can replace latitude / longitude / timezone with your own location.
const char *WEATHER_URL =
    "https://api.open-meteo.com/v1/forecast"
    "?latitude=31.2304"
    "&longitude=121.4737"
    "&current=temperature_2m,relative_humidity_2m"
    "&timezone=Asia%2FShanghai";

const char *NTP_SERVER_1 = "ntp1.aliyun.com";
const char *NTP_SERVER_2 = "ntp.ntsc.ac.cn";
const char *NTP_SERVER_3 = "pool.ntp.org";

constexpr uint8_t DIGIT_COUNT = 6;
// 18101BS common-anode display + BC327 high-side digit drivers:
// segments light when LOW, digit select is enabled when LOW.
// If all six digit positions stay on, try setting DIGITS_ACTIVE_LOW to false.
// If every digit looks like "8", try setting SEGMENTS_ACTIVE_LOW to false.
constexpr bool SEGMENTS_ACTIVE_LOW = true;
constexpr bool DIGITS_ACTIVE_LOW = true;

// true : NodeMCU -> 74HC595 #1(segments) -> 74HC595 #2(digits)
// false: NodeMCU -> 74HC595 #1(digits)   -> 74HC595 #2(segments)
constexpr bool FIRST_SHIFT_REGISTER_IS_SEGMENTS = true;

// 74HC595 control pins on NodeMCU(12F).
// D7/D5/D6 on the dev board map to IO13/IO14/IO12 in some schematic symbols.
constexpr uint8_t SHIFT_SER_PIN = D7;    // GPIO13 / IO13 -> SER
constexpr uint8_t SHIFT_SRCLK_PIN = D5;  // GPIO14 / IO14 -> SRCLK
constexpr uint8_t SHIFT_RCLK_PIN = D6;   // GPIO12 / IO12 -> RCLK

constexpr unsigned long REFRESH_INTERVAL_US = 1200;
constexpr unsigned int DIGIT_BLANKING_US = 40;
constexpr bool ENABLE_POWER_ON_DISPLAY_SELF_TEST = true;
constexpr unsigned int DISPLAY_SELF_TEST_STEP_MS = 120;
constexpr uint8_t DISPLAY_SELF_TEST_PASSES = 1;
constexpr unsigned long PAGE_INTERVAL_MS = 3500;
constexpr unsigned long CONTENT_INTERVAL_MS = 100;
constexpr unsigned long WIFI_RETRY_INTERVAL_MS = 10000;
constexpr unsigned long NTP_STATUS_INTERVAL_MS = 1000;
constexpr unsigned long WEATHER_INTERVAL_MS = 10UL * 60UL * 1000UL;
constexpr long TIMEZONE_OFFSET_SECONDS = 8L * 60L * 60L;
constexpr int DAYLIGHT_OFFSET_SECONDS = 0;
const time_t NTP_MIN_VALID_EPOCH = 1704067200;  // 2024-01-01 00:00:00 UTC

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
int lastLoggedWiFiStatus = -1;
uint8_t lastLoggedSecond = 255;
bool ntpConfigured = false;
bool ntpEverSynced = false;
unsigned long lastNtpStatusMs = 0;

const char *wifiStatusName(wl_status_t status) {
  switch (status) {
    case WL_IDLE_STATUS:
      return "IDLE";
    case WL_NO_SSID_AVAIL:
      return "NO_SSID";
    case WL_SCAN_COMPLETED:
      return "SCAN_DONE";
    case WL_CONNECTED:
      return "CONNECTED";
    case WL_CONNECT_FAILED:
      return "CONNECT_FAILED";
    case WL_CONNECTION_LOST:
      return "CONNECTION_LOST";
    case WL_DISCONNECTED:
      return "DISCONNECTED";
    default:
      return "UNKNOWN";
  }
}

void logStartupConfig() {
  Serial.println();
  Serial.println(F("[BOOT] Clock starting"));
  Serial.printf("[BOOT] WiFi SSID: %s\n", WIFI_SSID);
  Serial.printf("[BOOT] Weather URL: %s\n", WEATHER_URL);
  Serial.printf("[BOOT] Shift pins: SER=%u SRCLK=%u RCLK=%u\n", SHIFT_SER_PIN,
                SHIFT_SRCLK_PIN, SHIFT_RCLK_PIN);
  Serial.printf("[BOOT] NTP servers: %s, %s, %s\n", NTP_SERVER_1, NTP_SERVER_2,
                NTP_SERVER_3);
  Serial.printf("[BOOT] Timezone: UTC+%ld\n", TIMEZONE_OFFSET_SECONDS / 3600L);
}

void logWiFiStatusIfChanged() {
  wl_status_t status = WiFi.status();
  if (static_cast<int>(status) == lastLoggedWiFiStatus) {
    return;
  }

  lastLoggedWiFiStatus = static_cast<int>(status);
  Serial.printf("[WIFI] Status -> %s (%d)\n", wifiStatusName(status),
                static_cast<int>(status));

  if (status == WL_CONNECTED) {
    String ip = WiFi.localIP().toString();
    Serial.printf("[WIFI] Connected to %s, IP=%s, RSSI=%d dBm\n",
                  WiFi.SSID().c_str(), ip.c_str(), WiFi.RSSI());
  }
}

void configureNtp() {
  if (ntpConfigured) {
    return;
  }

  configTime(TIMEZONE_OFFSET_SECONDS, DAYLIGHT_OFFSET_SECONDS, NTP_SERVER_1,
             NTP_SERVER_2, NTP_SERVER_3);
  ntpConfigured = true;
  Serial.println(F("[NTP] SNTP configured"));
}

void startWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastWiFiRetryMs = millis();
  Serial.printf("[WIFI] Connecting to %s ...\n", WIFI_SSID);
}

bool isNtpTimeValid(time_t epoch) {
  return epoch >= NTP_MIN_VALID_EPOCH;
}

void updateNtpStatus() {
  unsigned long nowMs = millis();
  if (nowMs - lastNtpStatusMs < NTP_STATUS_INTERVAL_MS) {
    return;
  }
  lastNtpStatusMs = nowMs;

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  time_t epoch = time(nullptr);
  if (!isNtpTimeValid(epoch)) {
    return;
  }

  if (!ntpEverSynced) {
    tm timeInfo;
    localtime_r(&epoch, &timeInfo);
    Serial.printf("[NTP] Synced: %04d-%02d-%02d %02d:%02d:%02d\n",
                  timeInfo.tm_year + 1900, timeInfo.tm_mon + 1,
                  timeInfo.tm_mday, timeInfo.tm_hour, timeInfo.tm_min,
                  timeInfo.tm_sec);
  }
  ntpEverSynced = true;
}

void logLocalClockIfNeeded(const ClockTime &clock) {
  if (clock.second == lastLoggedSecond) {
    return;
  }

  lastLoggedSecond = clock.second;
  Serial.printf("[CLOCK] %04u-%02u-%02u %02u:%02u:%02u\n", clock.year, clock.month,
                clock.day, clock.hour, clock.minute, clock.second);
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

uint8_t makeSegmentRegisterValue(uint8_t mask) {
  return SEGMENTS_ACTIVE_LOW ? static_cast<uint8_t>(~mask) : mask;
}

uint8_t makeDigitRegisterValue(uint8_t index, bool enabled) {
  uint8_t value = DIGITS_ACTIVE_LOW ? 0xFF : 0x00;
  if (!enabled || index >= DIGIT_COUNT) {
    return value;
  }

  if (DIGITS_ACTIVE_LOW) {
    value &= static_cast<uint8_t>(~(1U << index));
  } else {
    value |= static_cast<uint8_t>(1U << index);
  }
  return value;
}

void writeShiftRegisters(uint8_t segmentState, uint8_t digitState) {
  uint8_t firstShiftedByte =
      FIRST_SHIFT_REGISTER_IS_SEGMENTS ? digitState : segmentState;
  uint8_t secondShiftedByte =
      FIRST_SHIFT_REGISTER_IS_SEGMENTS ? segmentState : digitState;

  digitalWrite(SHIFT_RCLK_PIN, LOW);
  shiftOut(SHIFT_SER_PIN, SHIFT_SRCLK_PIN, MSBFIRST, firstShiftedByte);
  shiftOut(SHIFT_SER_PIN, SHIFT_SRCLK_PIN, MSBFIRST, secondShiftedByte);
  digitalWrite(SHIFT_RCLK_PIN, HIGH);
}

void blankDisplayOutput() {
  writeShiftRegisters(makeSegmentRegisterValue(0),
                      makeDigitRegisterValue(0, false));
}

void runPowerOnDisplaySelfTest() {
  if (!ENABLE_POWER_ON_DISPLAY_SELF_TEST) {
    return;
  }

  const uint8_t allSegments = SEG_A | SEG_B | SEG_C | SEG_D |
                              SEG_E | SEG_F | SEG_G | SEG_DP;

  for (uint8_t pass = 0; pass < DISPLAY_SELF_TEST_PASSES; ++pass) {
    for (uint8_t digit = 0; digit < DIGIT_COUNT; ++digit) {
      writeShiftRegisters(makeSegmentRegisterValue(allSegments),
                          makeDigitRegisterValue(digit, true));
      delay(DISPLAY_SELF_TEST_STEP_MS);
    }
  }

  blankDisplayOutput();
}

void refreshDisplay() {
  unsigned long nowUs = micros();
  if (nowUs - lastRefreshUs < REFRESH_INTERVAL_US) {
    return;
  }
  lastRefreshUs = nowUs;

  blankDisplayOutput();
  delayMicroseconds(DIGIT_BLANKING_US);

  writeShiftRegisters(makeSegmentRegisterValue(displayBuffer[currentDigit]),
                      makeDigitRegisterValue(currentDigit, true));
  currentDigit = (currentDigit + 1) % DIGIT_COUNT;
}

bool getClockTime(ClockTime &clock) {
  time_t now = time(nullptr);
  if (!isNtpTimeValid(now)) {
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
  if (lastWiFiRetryMs != 0 &&
      nowMs - lastWiFiRetryMs < WIFI_RETRY_INTERVAL_MS) {
    return;
  }
  lastWiFiRetryMs = nowMs;

  Serial.printf("[WIFI] Connecting to %s ...\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

bool fetchWeatherNow() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  Serial.println(F("[WEATHER] Fetching current weather ..."));

  BearSSL::WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  if (!https.begin(client, WEATHER_URL)) {
    Serial.println(F("[WEATHER] HTTPS begin failed"));
    return false;
  }

  https.setTimeout(6000);
  int httpCode = https.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[WEATHER] HTTP GET failed: %d\n", httpCode);
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
  if (gotTemp || gotHumidity) {
    Serial.print(F("[WEATHER] Updated: temp="));
    if (gotTemp) {
      Serial.print(lastTemperature, 1);
      Serial.print(F("C"));
    } else {
      Serial.print(F("N/A"));
    }
    Serial.print(F(", humidity="));
    if (gotHumidity) {
      Serial.print(lastHumidity, 1);
      Serial.println(F("%"));
    } else {
      Serial.println(F("N/A"));
    }
  } else {
    Serial.println(F("[WEATHER] Parse failed: temperature_2m / relative_humidity_2m not found"));
  }
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
  if (timeOk) {
    showTime(now);
    logLocalClockIfNeeded(now);
  } else if (WiFi.status() == WL_CONNECTED) {
    showStatusPage('n', 't');
  } else {
    showStatusPage('n', 'o');
  }
}

void setup() {
  // Match the ESP8266 ROM boot log baud rate so startup output stays readable
  // in a single serial monitor session.
  Serial.begin(74880);
  delay(50);
  Serial.println();
  Serial.println(F("[BOOT] NTP clock mode"));
  logStartupConfig();
  Serial.printf("[BOOT] Display config: segmentsActiveLow=%s digitsActiveLow=%s first595=%s blanking=%u us\n",
                SEGMENTS_ACTIVE_LOW ? "true" : "false",
                DIGITS_ACTIVE_LOW ? "true" : "false",
                FIRST_SHIFT_REGISTER_IS_SEGMENTS ? "segments" : "digits",
                DIGIT_BLANKING_US);

  pinMode(SHIFT_SER_PIN, OUTPUT);
  pinMode(SHIFT_SRCLK_PIN, OUTPUT);
  pinMode(SHIFT_RCLK_PIN, OUTPUT);
  digitalWrite(SHIFT_SER_PIN, LOW);
  digitalWrite(SHIFT_SRCLK_PIN, LOW);
  digitalWrite(SHIFT_RCLK_PIN, HIGH);

  blankDisplayOutput();
  runPowerOnDisplaySelfTest();
  startWiFi();
  configureNtp();
}

void loop() {
  refreshDisplay();
  ensureWiFiConnected();
  logWiFiStatusIfChanged();
  updateNtpStatus();
  updateDisplayContent();
}
