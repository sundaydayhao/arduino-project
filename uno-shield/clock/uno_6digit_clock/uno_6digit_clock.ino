#include <DHT.h>
#include <math.h>
#include <string.h>

// ---------- User configuration ----------
// Fixed 6-digit layout from left to right:
// [0][1][2][3][4][5]
//  HH.MM.SS / YY.MM.DD / t_23.5C / h_56.2H
#define DIGIT_COUNT 6
#define SHIFT_DATA_PIN 2
#define SHIFT_CLOCK_PIN 3
#define SHIFT_LATCH_PIN 4
#define RTC_CE_PIN 5
#define RTC_IO_PIN 6
#define RTC_SCLK_PIN 7
#define DHTPIN 8
#define BUTTON_FUNCTION_PIN 9
#define BUTTON_SELECT_PIN 10
#define BUTTON_UP_PIN 11
#define BUTTON_DOWN_PIN 12
#define DHTTYPE DHT11   // Change to DHT22 if you use DHT22.

// false: common cathode digits + NPN low-side digit transistors
// true : common anode digits + PNP/P-MOS high-side digit transistors (18101BS default)
const bool COMMON_ANODE = true;
const bool ENABLE_SENSOR_PAGES = true;
const bool ENABLE_SERIAL_DEBUG = true;
const bool FORCE_RTC_SET_FROM_COMPILE_TIME_ON_BOOT = false;
const bool SET_RTC_FROM_COMPILE_TIME_IF_RTC_IS_OLDER = true;
const bool TIME_SEPARATOR_DOTS_ALWAYS_ON = true;
const bool DEBUG_PRINT_EVERY_RTC_SECOND = false;

const unsigned long REFRESH_INTERVAL_US = 1000;
const unsigned long AUTO_DISPLAY_CYCLE_MS = 60000;
const unsigned long AUTO_DATE_DISPLAY_MS = 5000;
const unsigned long AUTO_TEMP_DISPLAY_MS = 5000;
const unsigned long AUTO_HUMI_DISPLAY_MS = 5000;
const unsigned long SENSOR_INTERVAL_MS = 10000;
const unsigned long CONTENT_INTERVAL_MS = 100;
const unsigned long RTC_POLL_INTERVAL_MS = 1000;
const unsigned long BUTTON_DEBOUNCE_MS = 30;
const unsigned long BUTTON_LONG_PRESS_MS = 800;
const unsigned long BUTTON_REPEAT_START_MS = 500;
const unsigned long BUTTON_REPEAT_INTERVAL_MS = 180;
const unsigned long MANUAL_PAGE_HOLD_MS = 10000;
const bool ENABLE_POWER_ON_DISPLAY_SELF_TEST = true;
const unsigned int DISPLAY_SELF_TEST_STEP_MS = 90;
const uint8_t DISPLAY_SELF_TEST_PASSES = 2;
const unsigned int DIGIT_BLANKING_US = 40;
const unsigned long SERIAL_DEBUG_BAUD = 115200;
const uint8_t SERIAL_COMMAND_BUFFER_SIZE = 40;
const uint8_t RTC_READ_RETRY_COUNT = 5;
const unsigned int RTC_READ_RETRY_DELAY_US = 500;

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

const uint8_t VIEW_PAGE_COUNT = ENABLE_SENSOR_PAGES ? PAGE_COUNT : PAGE_TEMP;

enum ButtonId : uint8_t {
  BUTTON_NONE = 0,
  BUTTON_FUNCTION = 1,
  BUTTON_SELECT = 2,
  BUTTON_UP = 3,
  BUTTON_DOWN = 4
};

// Two 74HC595 are daisy-chained:
// Arduino -> segment 595 -> digit-select 595.
// The first shifted byte reaches the far register (digit-select),
// the second shifted byte stays in the near register (segments).
const uint8_t buttonPins[BUTTON_DOWN + 1] = {
  0,
  BUTTON_FUNCTION_PIN,
  BUTTON_SELECT_PIN,
  BUTTON_UP_PIN,
  BUTTON_DOWN_PIN
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
unsigned long lastRtcPollMs = 0;

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
char serialCommandBuffer[SERIAL_COMMAND_BUFFER_SIZE] = {0};
uint8_t serialCommandLength = 0;
ClockTime cachedRtcTime = {2026, 1, 1, 1, 0, 0, 0};
bool hasCachedRtcTime = false;
unsigned long cachedRtcMillis = 0;

const uint8_t DS1302_REG_WP = 0x8E;
const uint8_t DS1302_CMD_CLOCK_BURST_READ = 0xBF;
const uint8_t DS1302_CMD_CLOCK_BURST_WRITE = 0xBE;

const char *pageName(Page page) {
  switch (page) {
    case PAGE_TIME:
      return "time";
    case PAGE_DATE:
      return "date";
    case PAGE_TEMP:
      return "temp";
    case PAGE_HUMI:
      return "humi";
    default:
      return "unknown";
  }
}

const char *buttonName(ButtonId button) {
  switch (button) {
    case BUTTON_FUNCTION:
      return "function";
    case BUTTON_SELECT:
      return "select";
    case BUTTON_UP:
      return "up";
    case BUTTON_DOWN:
      return "down";
    default:
      return "none";
  }
}

const char *editFieldName(EditField field) {
  switch (field) {
    case EDIT_HOUR:
      return "hour";
    case EDIT_MINUTE:
      return "minute";
    case EDIT_SECOND:
      return "second";
    case EDIT_YEAR:
      return "year";
    case EDIT_MONTH:
      return "month";
    case EDIT_DAY:
      return "day";
    default:
      return "unknown";
  }
}

void debugPrintTwoDigits(uint8_t value) {
  if (value < 10) {
    Serial.print('0');
  }
  Serial.print(value);
}

void debugPrintDateTime(const ClockTime &time) {
  Serial.print(time.year);
  Serial.print('-');
  debugPrintTwoDigits(time.month);
  Serial.print('-');
  debugPrintTwoDigits(time.day);
  Serial.print(' ');
  debugPrintTwoDigits(time.hour);
  Serial.print(':');
  debugPrintTwoDigits(time.minute);
  Serial.print(':');
  debugPrintTwoDigits(time.second);
}

void debugPrintConfig() {
  if (!ENABLE_SERIAL_DEBUG) {
    return;
  }

  Serial.print(F("[clock] config commonAnode="));
  Serial.print(COMMON_ANODE ? F("true") : F("false"));
  Serial.print(F(" sensorPages="));
  Serial.print(ENABLE_SENSOR_PAGES ? F("true") : F("false"));
  Serial.print(F(" viewPages="));
  Serial.print(VIEW_PAGE_COUNT);
  Serial.print(F(" cycleMs="));
  Serial.print(AUTO_DISPLAY_CYCLE_MS);
  Serial.print(F(" dateMs="));
  Serial.print(AUTO_DATE_DISPLAY_MS);
  Serial.print(F(" tempMs="));
  Serial.print(ENABLE_SENSOR_PAGES ? AUTO_TEMP_DISPLAY_MS : 0);
  Serial.print(F(" humiMs="));
  Serial.print(ENABLE_SENSOR_PAGES ? AUTO_HUMI_DISPLAY_MS : 0);
  Serial.print(F(" refreshUs="));
  Serial.print(REFRESH_INTERVAL_US);
  Serial.print(F(" blankingUs="));
  Serial.print(DIGIT_BLANKING_US);
  Serial.print(F(" selfTest="));
  Serial.print(ENABLE_POWER_ON_DISPLAY_SELF_TEST ? F("true") : F("false"));
  Serial.print(F(" forceRtcCompile="));
  Serial.print(FORCE_RTC_SET_FROM_COMPILE_TIME_ON_BOOT ? F("true") : F("false"));
  Serial.print(F(" syncOldRtc="));
  Serial.print(SET_RTC_FROM_COMPILE_TIME_IF_RTC_IS_OLDER ? F("true") : F("false"));
  Serial.print(F(" timeDotsAlwaysOn="));
  Serial.print(TIME_SEPARATOR_DOTS_ALWAYS_ON ? F("true") : F("false"));
  Serial.print(F(" rtcPollMs="));
  Serial.print(RTC_POLL_INTERVAL_MS);
  Serial.print(F(" debugEverySecond="));
  Serial.println(DEBUG_PRINT_EVERY_RTC_SECOND ? F("true") : F("false"));
}

void debugPrintSerialHelp() {
  if (!ENABLE_SERIAL_DEBUG) {
    return;
  }

  Serial.println(F("[clock] serial commands:"));
  Serial.println(F("[clock]   TIME?"));
  Serial.println(F("[clock]   SET 2026-04-28 23:05:00"));
  Serial.println(F("[clock]   COMPILE"));
}

void debugPrintRtcStatus(const char *label,
                         const ClockTime &time,
                         bool rtcReadOk,
                         bool clockHalted) {
  if (!ENABLE_SERIAL_DEBUG) {
    return;
  }

  Serial.print(F("[clock] "));
  Serial.print(label);
  Serial.print(F(" rtcReadOk="));
  Serial.print(rtcReadOk ? F("true") : F("false"));
  Serial.print(F(" halted="));
  Serial.print(clockHalted ? F("true") : F("false"));
  if (rtcReadOk) {
    Serial.print(F(" time="));
    debugPrintDateTime(time);
  }
  Serial.println();
}

void debugPrintButtonEvent(ButtonId button, bool isLong) {
  if (!ENABLE_SERIAL_DEBUG) {
    return;
  }

  Serial.print(F("[clock] button "));
  Serial.print(buttonName(button));
  Serial.print(isLong ? F(" long") : F(" short"));
  Serial.print(F(" mode="));
  Serial.println(uiMode == UI_EDIT ? F("edit") : F("view"));
}

void debugPrintEditState(const char *action) {
  if (!ENABLE_SERIAL_DEBUG) {
    return;
  }

  Serial.print(F("[clock] edit "));
  Serial.print(action);
  Serial.print(F(" field="));
  Serial.print(editFieldName(editField));
  Serial.print(F(" value="));
  debugPrintDateTime(editTime);
  Serial.println();
}

void debugReportDisplayState(Page page,
                             bool rtcOk,
                             bool clockHalted,
                             const ClockTime &now) {
  if (!ENABLE_SERIAL_DEBUG) {
    return;
  }

  static bool hasLastState = false;
  static UiMode lastMode = UI_VIEW;
  static Page lastPage = PAGE_COUNT;
  static bool lastRtcOk = false;
  static bool lastClockHalted = false;
  static bool lastPrintedHadRtcTime = false;
  static ClockTime lastPrintedRtcTime = {2000, 1, 1, 1, 0, 0, 0};

  bool stateChanged = !hasLastState ||
                      lastMode != uiMode ||
                      lastPage != page ||
                      lastRtcOk != rtcOk ||
                      lastClockHalted != clockHalted;

  bool rtcTimeChanged = false;
  if (DEBUG_PRINT_EVERY_RTC_SECOND && rtcOk) {
    rtcTimeChanged = !lastPrintedHadRtcTime ||
                     lastPrintedRtcTime.year != now.year ||
                     lastPrintedRtcTime.month != now.month ||
                     lastPrintedRtcTime.day != now.day ||
                     lastPrintedRtcTime.hour != now.hour ||
                     lastPrintedRtcTime.minute != now.minute ||
                     lastPrintedRtcTime.second != now.second;
  }

  if (!stateChanged && !rtcTimeChanged) {
    return;
  }

  hasLastState = true;
  lastMode = uiMode;
  lastPage = page;
  lastRtcOk = rtcOk;
  lastClockHalted = clockHalted;
  if (rtcOk) {
    lastPrintedRtcTime = now;
    lastPrintedHadRtcTime = true;
  } else {
    lastPrintedHadRtcTime = false;
  }

  Serial.print(F("[clock] display mode="));
  Serial.print(uiMode == UI_EDIT ? F("edit") : F("view"));
  Serial.print(F(" page="));
  Serial.print(pageName(page));
  Serial.print(F(" rtc="));
  Serial.print(rtcOk ? F("ok") : (clockHalted ? F("halted") : F("invalid")));
  if (rtcOk) {
    Serial.print(F(" time="));
    debugPrintDateTime(now);
  }
  Serial.println();
}

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

uint32_t secondsSince2000(const ClockTime &time);
bool isBefore(const ClockTime &left, const ClockTime &right);
void cacheRtcTime(const ClockTime &time);
bool readCachedRtcTime(ClockTime &time);

bool readRtcRaw(ClockTime &time, bool &clockHalted) {
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

bool isRtcTimePlausible(const ClockTime &time) {
  if (!SET_RTC_FROM_COMPILE_TIME_IF_RTC_IS_OLDER) {
    return true;
  }

  ClockTime compileTime = compileTimeToClockTime();
  return !isBefore(time, compileTime);
}

bool readRtc(ClockTime &time, bool &clockHalted) {
  ClockTime haltedTime;
  bool sawPlausibleHaltedTime = false;

  for (uint8_t attempt = 0; attempt < RTC_READ_RETRY_COUNT; ++attempt) {
    ClockTime candidate;
    bool candidateHalted = false;
    bool candidateValid = readRtcRaw(candidate, candidateHalted);
    bool candidatePlausible = candidateValid && isRtcTimePlausible(candidate);

    if (candidatePlausible && !candidateHalted) {
      time = candidate;
      clockHalted = false;
      cacheRtcTime(time);
      return true;
    }

    if (candidatePlausible && candidateHalted && !sawPlausibleHaltedTime) {
      haltedTime = candidate;
      sawPlausibleHaltedTime = true;
    }

    delayMicroseconds(RTC_READ_RETRY_DELAY_US);
  }

  if (sawPlausibleHaltedTime) {
    if (readCachedRtcTime(time)) {
      clockHalted = false;
      return true;
    }

    time = haltedTime;
    clockHalted = true;
    return true;
  }

  if (readCachedRtcTime(time)) {
    clockHalted = false;
    return true;
  }

  clockHalted = false;
  return false;
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
  cacheRtcTime(time);
}

void advanceClockTime(ClockTime &time, uint32_t secondsToAdd) {
  uint32_t totalSeconds = time.second + secondsToAdd;
  time.second = totalSeconds % 60;

  uint32_t totalMinutes = time.minute + (totalSeconds / 60);
  time.minute = totalMinutes % 60;

  uint32_t totalHours = time.hour + (totalMinutes / 60);
  time.hour = totalHours % 24;

  uint32_t daysToAdd = totalHours / 24;
  while (daysToAdd > 0) {
    ++time.day;
    if (time.day > daysInMonth(time.year, time.month)) {
      time.day = 1;
      ++time.month;
      if (time.month > 12) {
        time.month = 1;
        ++time.year;
      }
    }
    --daysToAdd;
  }

  time.dayOfWeek = calculateDayOfWeek(time.year, time.month, time.day);
}

void cacheRtcTime(const ClockTime &time) {
  cachedRtcTime = time;
  cachedRtcMillis = millis();
  hasCachedRtcTime = true;
}

bool readCachedRtcTime(ClockTime &time) {
  if (!hasCachedRtcTime) {
    return false;
  }

  time = cachedRtcTime;
  advanceClockTime(time, (millis() - cachedRtcMillis) / 1000UL);
  return isValidClockTime(time);
}

uint32_t secondsSince2000(const ClockTime &time) {
  uint32_t days = 0;
  for (uint16_t year = 2000; year < time.year; ++year) {
    days += isLeapYear(year) ? 366UL : 365UL;
  }
  for (uint8_t month = 1; month < time.month; ++month) {
    days += daysInMonth(time.year, month);
  }
  days += time.day - 1;

  return days * 86400UL +
         static_cast<uint32_t>(time.hour) * 3600UL +
         static_cast<uint32_t>(time.minute) * 60UL +
         time.second;
}

bool isBefore(const ClockTime &left, const ClockTime &right) {
  return secondsSince2000(left) < secondsSince2000(right);
}

bool parseFixedDigits(const char *&cursor, uint8_t digitCount, uint16_t &value) {
  value = 0;
  for (uint8_t i = 0; i < digitCount; ++i) {
    char ch = cursor[i];
    if (ch < '0' || ch > '9') {
      return false;
    }
    value = value * 10 + (ch - '0');
  }
  cursor += digitCount;
  return true;
}

bool consumeChar(const char *&cursor, char expected) {
  if (*cursor != expected) {
    return false;
  }
  ++cursor;
  return true;
}

bool parseSetTimeCommand(const char *command, ClockTime &time) {
  if (strncmp(command, "SET ", 4) != 0) {
    return false;
  }

  const char *cursor = command + 4;
  uint16_t year;
  uint16_t month;
  uint16_t day;
  uint16_t hour;
  uint16_t minute;
  uint16_t second;

  if (!parseFixedDigits(cursor, 4, year) ||
      !consumeChar(cursor, '-') ||
      !parseFixedDigits(cursor, 2, month) ||
      !consumeChar(cursor, '-') ||
      !parseFixedDigits(cursor, 2, day) ||
      !consumeChar(cursor, ' ') ||
      !parseFixedDigits(cursor, 2, hour) ||
      !consumeChar(cursor, ':') ||
      !parseFixedDigits(cursor, 2, minute) ||
      !consumeChar(cursor, ':') ||
      !parseFixedDigits(cursor, 2, second)) {
    return false;
  }

  while (*cursor == ' ') {
    ++cursor;
  }
  if (*cursor != '\0') {
    return false;
  }

  ClockTime parsed = {
    year,
    static_cast<uint8_t>(month),
    static_cast<uint8_t>(day),
    1,
    static_cast<uint8_t>(hour),
    static_cast<uint8_t>(minute),
    static_cast<uint8_t>(second)
  };

  if (parsed.year < 2000 || parsed.year > 2099 ||
      parsed.month < 1 || parsed.month > 12 ||
      parsed.day < 1 || parsed.day > 31 ||
      parsed.hour > 23 || parsed.minute > 59 || parsed.second > 59) {
    return false;
  }

  parsed.dayOfWeek = calculateDayOfWeek(parsed.year, parsed.month, parsed.day);

  if (!isValidClockTime(parsed)) {
    return false;
  }

  time = parsed;
  return true;
}

void printRtcNow(const char *label) {
  ClockTime now;
  bool clockHalted = false;
  bool rtcReadOk = readRtc(now, clockHalted);
  debugPrintRtcStatus(label, now, rtcReadOk, clockHalted);
}

void processSerialCommand(const char *command) {
  if (!ENABLE_SERIAL_DEBUG || command[0] == '\0') {
    return;
  }

  if (strcmp(command, "HELP") == 0) {
    debugPrintSerialHelp();
    return;
  }

  if (strcmp(command, "TIME?") == 0 || strcmp(command, "READ") == 0) {
    printRtcNow("rtc serial read");
    return;
  }

  if (strcmp(command, "COMPILE") == 0) {
    ClockTime compileTime = compileTimeToClockTime();
    writeRtc(compileTime);
    Serial.print(F("[clock] rtc serial set compileTime="));
    debugPrintDateTime(compileTime);
    Serial.println();
    printRtcNow("rtc after serial set");
    return;
  }

  ClockTime requestedTime;
  if (parseSetTimeCommand(command, requestedTime)) {
    writeRtc(requestedTime);
    Serial.print(F("[clock] rtc serial set time="));
    debugPrintDateTime(requestedTime);
    Serial.println();
    printRtcNow("rtc after serial set");
    return;
  }

  Serial.print(F("[clock] unknown command: "));
  Serial.println(command);
  debugPrintSerialHelp();
}

void updateSerialCommands() {
  if (!ENABLE_SERIAL_DEBUG) {
    return;
  }

  while (Serial.available() > 0) {
    char ch = static_cast<char>(Serial.read());
    if (ch == '\r') {
      continue;
    }

    if (ch == '\n') {
      serialCommandBuffer[serialCommandLength] = '\0';
      processSerialCommand(serialCommandBuffer);
      serialCommandLength = 0;
      serialCommandBuffer[0] = '\0';
      continue;
    }

    if (serialCommandLength + 1 >= SERIAL_COMMAND_BUFFER_SIZE) {
      serialCommandLength = 0;
      serialCommandBuffer[0] = '\0';
      Serial.println(F("[clock] serial command too long"));
      continue;
    }

    serialCommandBuffer[serialCommandLength++] = ch;
  }
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

  unsigned long cycleMs = nowMs % AUTO_DISPLAY_CYCLE_MS;
  unsigned long tailStartMs = AUTO_DISPLAY_CYCLE_MS;

  if (ENABLE_SENSOR_PAGES) {
    tailStartMs -= AUTO_HUMI_DISPLAY_MS;
    if (cycleMs >= tailStartMs) {
      return PAGE_HUMI;
    }

    tailStartMs -= AUTO_TEMP_DISPLAY_MS;
    if (cycleMs >= tailStartMs) {
      return PAGE_TEMP;
    }
  }

  tailStartMs -= AUTO_DATE_DISPLAY_MS;
  if (cycleMs >= tailStartMs) {
    return PAGE_DATE;
  }

  return PAGE_TIME;
}

void holdManualPage(Page page) {
  manualPage = page;
  manualPageUntilMs = millis() + MANUAL_PAGE_HOLD_MS;
  if (ENABLE_SERIAL_DEBUG) {
    Serial.print(F("[clock] hold page="));
    Serial.print(pageName(page));
    Serial.print(F(" untilMs="));
    Serial.println(manualPageUntilMs);
  }
}

void startEditMode(const ClockTime &currentTime) {
  editTime = currentTime;
  normalizeEditDate();
  editField = EDIT_HOUR;
  uiMode = UI_EDIT;
  debugPrintEditState("start");
}

void saveEditTime() {
  normalizeEditDate();
  debugPrintEditState("save");
  writeRtc(editTime);
  uiMode = UI_VIEW;
  holdManualPage(PAGE_TIME);
}

void cancelEditMode() {
  uiMode = UI_VIEW;
  debugPrintEditState("cancel");
  holdManualPage(PAGE_TIME);
}

void advanceEditField() {
  editField = static_cast<EditField>((editField + 1) % EDIT_COUNT);
  debugPrintEditState("field");
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
  debugPrintEditState(delta > 0 ? "up" : "down");
}

ButtonId readButtonRaw() {
  for (uint8_t button = BUTTON_FUNCTION; button <= BUTTON_DOWN; ++button) {
    if (digitalRead(buttonPins[button]) == LOW) {
      return static_cast<ButtonId>(button);
    }
  }
  return BUTTON_NONE;
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

void setTwoDigitValue(uint8_t startIndex, uint8_t value, bool secondDigitWithDp = false) {
  if (startIndex + 1 >= DIGIT_COUNT) {
    return;
  }

  setGlyph(startIndex, '0' + (value / 10));
  setGlyph(startIndex + 1, '0' + (value % 10), secondDigitWithDp);
}

uint8_t makeSegmentRegisterValue(uint8_t mask) {
  return COMMON_ANODE ? static_cast<uint8_t>(~mask) : mask;
}

uint8_t makeDigitRegisterValue(uint8_t index, bool enabled) {
  uint8_t value = COMMON_ANODE ? 0xFF : 0x00;
  if (!enabled || index >= DIGIT_COUNT) {
    return value;
  }

  if (COMMON_ANODE) {
    value &= ~(1 << index);
  } else {
    value |= (1 << index);
  }
  return value;
}

void writeShiftRegisters(uint8_t digitValue, uint8_t segmentValue) {
  digitalWrite(SHIFT_LATCH_PIN, LOW);
  shiftOut(SHIFT_DATA_PIN, SHIFT_CLOCK_PIN, MSBFIRST, digitValue);
  shiftOut(SHIFT_DATA_PIN, SHIFT_CLOCK_PIN, MSBFIRST, segmentValue);
  digitalWrite(SHIFT_LATCH_PIN, HIGH);
}

void blankDisplayOutputs() {
  writeShiftRegisters(makeDigitRegisterValue(0, false), makeSegmentRegisterValue(0));
}

void runPowerOnDisplaySelfTest() {
  if (!ENABLE_POWER_ON_DISPLAY_SELF_TEST) {
    return;
  }

  const uint8_t allSegments = SEG_A | SEG_B | SEG_C | SEG_D |
                              SEG_E | SEG_F | SEG_G | SEG_DP;

  for (uint8_t pass = 0; pass < DISPLAY_SELF_TEST_PASSES; ++pass) {
    for (uint8_t digit = 0; digit < DIGIT_COUNT; ++digit) {
      writeShiftRegisters(
        makeDigitRegisterValue(digit, true),
        makeSegmentRegisterValue(allSegments)
      );
      delay(DISPLAY_SELF_TEST_STEP_MS);
    }
  }

  blankDisplayOutputs();
}

void refreshDisplay() {
  unsigned long nowUs = micros();
  if (nowUs - lastRefreshUs < REFRESH_INTERVAL_US) {
    return;
  }
  lastRefreshUs = nowUs;

  blankDisplayOutputs();
  delayMicroseconds(DIGIT_BLANKING_US);

  writeShiftRegisters(
    makeDigitRegisterValue(currentDigit, true),
    makeSegmentRegisterValue(displayBuffer[currentDigit])
  );
  currentDigit = (currentDigit + 1) % DIGIT_COUNT;
}

void showTime(const ClockTime &now) {
  clearDisplayBuffer();

  bool showSeparators = TIME_SEPARATOR_DOTS_ALWAYS_ON || (now.second % 2) == 0;
  setTwoDigitValue(0, now.hour, showSeparators);
  setTwoDigitValue(2, now.minute, showSeparators);
  setTwoDigitValue(4, now.second);
}

void showDate(const ClockTime &now) {
  clearDisplayBuffer();

  uint8_t year2 = now.year % 100;
  setTwoDigitValue(0, year2, true);
  setTwoDigitValue(2, now.month, true);
  setTwoDigitValue(4, now.day);
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

  if (ENABLE_SERIAL_DEBUG) {
    Serial.print(F("[clock] dht temperature="));
    if (isnan(t)) {
      Serial.print(F("nan"));
    } else {
      Serial.print(t, 1);
    }
    Serial.print(F(" humidity="));
    if (isnan(h)) {
      Serial.println(F("nan"));
    } else {
      Serial.println(h, 1);
    }
  }

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

  debugPrintButtonEvent(button, isLong);

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
    holdManualPage(static_cast<Page>((page + 1) % VIEW_PAGE_COUNT));
    return;
  }

  if (button == BUTTON_UP && !isLong) {
    holdManualPage(static_cast<Page>((page + VIEW_PAGE_COUNT - 1) % VIEW_PAGE_COUNT));
    return;
  }

  if (button == BUTTON_DOWN && !isLong) {
    holdManualPage(static_cast<Page>((page + 1) % VIEW_PAGE_COUNT));
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
  bool rtcReadOk = false;
  bool shouldPollRtc = !hasCachedRtcTime ||
                       lastRtcPollMs == 0 ||
                       nowMs - lastRtcPollMs >= RTC_POLL_INTERVAL_MS;

  if (shouldPollRtc) {
    rtcReadOk = readRtc(now, clockHalted);
    lastRtcPollMs = nowMs;
  } else {
    rtcReadOk = readCachedRtcTime(now);
    clockHalted = false;
  }

  bool rtcOk = rtcReadOk && !clockHalted;
  debugReportDisplayState(page, rtcOk, clockHalted, now);

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
  bool rtcReadOk = readRtc(now, clockHalted);
  ClockTime compileTime = compileTimeToClockTime();
  bool rtcOlderThanCompile = rtcReadOk &&
                             !clockHalted &&
                             SET_RTC_FROM_COMPILE_TIME_IF_RTC_IS_OLDER &&
                             isBefore(now, compileTime);

  debugPrintRtcStatus("rtc boot", now, rtcReadOk, clockHalted);

  if (FORCE_RTC_SET_FROM_COMPILE_TIME_ON_BOOT ||
      !rtcReadOk ||
      clockHalted ||
      rtcOlderThanCompile) {
    if (ENABLE_SERIAL_DEBUG) {
      Serial.print(F("[clock] rtc set compileTime reason="));
      if (FORCE_RTC_SET_FROM_COMPILE_TIME_ON_BOOT) {
        Serial.print(F("force"));
      } else if (clockHalted) {
        Serial.print(F("halted"));
      } else if (rtcOlderThanCompile) {
        Serial.print(F("older-than-compile"));
      } else {
        Serial.print(F("invalid"));
      }
      Serial.print(F(" time="));
      debugPrintDateTime(compileTime);
      Serial.println();
    }
    writeRtc(compileTime);

    clockHalted = false;
    rtcReadOk = readRtc(now, clockHalted);
    debugPrintRtcStatus("rtc after set", now, rtcReadOk, clockHalted);
  }

  // For manual calibration, open Serial Monitor at 115200 and send:
  // SET 2026-04-28 23:05:00
}

void setup() {
  if (ENABLE_SERIAL_DEBUG) {
    Serial.begin(SERIAL_DEBUG_BAUD);
    delay(100);
    Serial.println();
    Serial.println(F("[clock] boot"));
    debugPrintConfig();
    debugPrintSerialHelp();
  }

  pinMode(SHIFT_DATA_PIN, OUTPUT);
  pinMode(SHIFT_CLOCK_PIN, OUTPUT);
  pinMode(SHIFT_LATCH_PIN, OUTPUT);
  digitalWrite(SHIFT_DATA_PIN, LOW);
  digitalWrite(SHIFT_CLOCK_PIN, LOW);
  digitalWrite(SHIFT_LATCH_PIN, HIGH);

  for (uint8_t button = BUTTON_FUNCTION; button <= BUTTON_DOWN; ++button) {
    pinMode(buttonPins[button], INPUT_PULLUP);
  }

  blankDisplayOutputs();
  runPowerOnDisplaySelfTest();

  if (ENABLE_SENSOR_PAGES) {
    dht.begin();
  }
  setupRtc();
}

void loop() {
  refreshDisplay();
  updateButtons();
  handleButtonEvent();
  updateSerialCommands();
  if (ENABLE_SENSOR_PAGES) {
    updateSensors();
  }
  updateDisplayContent();
}
