#include <Arduino.h>

enum LampColor : uint8_t {
  RED,
  YELLOW,
  GREEN
};

enum Direction : uint8_t {
  NORTH,
  SOUTH,
  EAST,
  WEST,
  DIRECTION_COUNT
};

enum GroupId : uint8_t {
  NS_GROUP,
  EW_GROUP
};

enum SegmentId : uint8_t {
  SEG_A,
  SEG_B,
  SEG_C,
  SEG_D,
  SEG_E,
  SEG_F,
  SEG_G,
  SEG_DP,
  SEGMENT_COUNT
};

enum PhysicalDigit : uint8_t {
  NORTH_TENS,
  NORTH_ONES,
  SOUTH_TENS,
  SOUTH_ONES,
  EAST_TENS,
  EAST_ONES,
  WEST_TENS,
  WEST_ONES,
  PHYSICAL_DIGIT_COUNT
};

struct LightPins {
  uint8_t red;
  uint8_t yellow;
  uint8_t green;
};

struct StageDefinition {
  const char* name;
  uint8_t durationSeconds;
  LampColor nsColor;
  LampColor ewColor;
};

// 3621BH common-anode pinout, based on the XLITX CL3621BH datasheet:
// A=3, B=9, C=8, D=6, E=7, F=4, G=1, DP=2, DIG1=10, DIG2=5
//
// Hardware assumptions for this example:
// 1. Segment lines are shared across all displays and drive ULN2803 inputs.
//    A segment lights when its Arduino pin outputs HIGH.
// 2. Digit select lines drive PNP/P-MOS high-side switches.
//    A digit turns on when its Arduino pin outputs LOW.

static const LightPins LIGHT_PINS[DIRECTION_COUNT] = {
    {2, 3, 4},      // North
    {5, 6, 7},      // South
    {8, 9, 10},     // East
    {11, 12, 13},   // West
};

static const uint8_t SEGMENT_PINS[SEGMENT_COUNT] = {
    22,  // A
    23,  // B
    24,  // C
    25,  // D
    26,  // E
    27,  // F
    28,  // G
    29,  // DP
};

static const uint8_t DIGIT_PINS[PHYSICAL_DIGIT_COUNT] = {
    30,  // North tens
    31,  // North ones
    32,  // South tens
    33,  // South ones
    34,  // East tens
    35,  // East ones
    36,  // West tens
    37,  // West ones
};

static const StageDefinition STAGES[] = {
    {"NS_GREEN", 15, GREEN, RED},
    {"NS_YELLOW", 3, YELLOW, RED},
    {"ALL_RED_TO_EW", 2, RED, RED},
    {"EW_GREEN", 15, RED, GREEN},
    {"EW_YELLOW", 3, RED, YELLOW},
    {"ALL_RED_TO_NS", 2, RED, RED},
};

static constexpr uint8_t STAGE_COUNT = sizeof(STAGES) / sizeof(STAGES[0]);
static constexpr uint8_t BLANK_DIGIT = 0xFF;

class TrafficLightHead {
 public:
  explicit TrafficLightHead(const LightPins& pins) : pins_(pins) {}

  void begin() const {
    pinMode(pins_.red, OUTPUT);
    pinMode(pins_.yellow, OUTPUT);
    pinMode(pins_.green, OUTPUT);
    allOff();
  }

  void setColor(LampColor color) const {
    digitalWrite(pins_.red, color == RED ? HIGH : LOW);
    digitalWrite(pins_.yellow, color == YELLOW ? HIGH : LOW);
    digitalWrite(pins_.green, color == GREEN ? HIGH : LOW);
  }

  void allOff() const {
    digitalWrite(pins_.red, LOW);
    digitalWrite(pins_.yellow, LOW);
    digitalWrite(pins_.green, LOW);
  }

 private:
  LightPins pins_;
};

class CountdownDisplayScanner {
 public:
  void begin() {
    for (uint8_t i = 0; i < SEGMENT_COUNT; ++i) {
      pinMode(SEGMENT_PINS[i], OUTPUT);
      digitalWrite(SEGMENT_PINS[i], LOW);
    }

    for (uint8_t i = 0; i < PHYSICAL_DIGIT_COUNT; ++i) {
      pinMode(DIGIT_PINS[i], OUTPUT);
      digitalWrite(DIGIT_PINS[i], HIGH);
      digitValues_[i] = 0;
    }
  }

  void setNumber(Direction direction, uint8_t value) {
    if (value > 99) {
      value = 99;
    }

    const uint8_t tens = value / 10;
    const uint8_t ones = value % 10;
    const uint8_t baseIndex = static_cast<uint8_t>(direction) * 2;

    digitValues_[baseIndex] = tens;
    digitValues_[baseIndex + 1] = ones;
  }

  void blankDirection(Direction direction) {
    const uint8_t baseIndex = static_cast<uint8_t>(direction) * 2;
    digitValues_[baseIndex] = BLANK_DIGIT;
    digitValues_[baseIndex + 1] = BLANK_DIGIT;
  }

  void service() {
    const unsigned long nowUs = micros();
    if (nowUs - lastScanUs_ < SCAN_INTERVAL_US) {
      return;
    }

    lastScanUs_ = nowUs;
    disableAllDigits();
    writeSegments(encodeDigit(digitValues_[activeDigit_]));
    digitalWrite(DIGIT_PINS[activeDigit_], LOW);
    activeDigit_ = (activeDigit_ + 1) % PHYSICAL_DIGIT_COUNT;
  }

 private:
  static constexpr unsigned long SCAN_INTERVAL_US = 1000UL;

  void disableAllDigits() const {
    for (uint8_t i = 0; i < PHYSICAL_DIGIT_COUNT; ++i) {
      digitalWrite(DIGIT_PINS[i], HIGH);
    }
  }

  void writeSegments(uint8_t pattern) const {
    for (uint8_t i = 0; i < SEGMENT_COUNT; ++i) {
      const bool segmentOn = (pattern >> i) & 0x01;
      digitalWrite(SEGMENT_PINS[i], segmentOn ? HIGH : LOW);
    }
  }

  uint8_t encodeDigit(uint8_t digit) const {
    switch (digit) {
      case 0:
        return 0b00111111;
      case 1:
        return 0b00000110;
      case 2:
        return 0b01011011;
      case 3:
        return 0b01001111;
      case 4:
        return 0b01100110;
      case 5:
        return 0b01101101;
      case 6:
        return 0b01111101;
      case 7:
        return 0b00000111;
      case 8:
        return 0b01111111;
      case 9:
        return 0b01101111;
      default:
        return 0;
    }
  }

  uint8_t digitValues_[PHYSICAL_DIGIT_COUNT] = {0};
  uint8_t activeDigit_ = 0;
  unsigned long lastScanUs_ = 0;
};

TrafficLightHead northLight(LIGHT_PINS[NORTH]);
TrafficLightHead southLight(LIGHT_PINS[SOUTH]);
TrafficLightHead eastLight(LIGHT_PINS[EAST]);
TrafficLightHead westLight(LIGHT_PINS[WEST]);
CountdownDisplayScanner countdownScanner;

class IntersectionController {
 public:
  void begin() {
    northLight.begin();
    southLight.begin();
    eastLight.begin();
    westLight.begin();

    countdownScanner.begin();

    currentStageIndex_ = 0;
    stageStartMs_ = millis();
    applyOutputs();
    refreshCountdowns();
  }

  void update() {
    const unsigned long now = millis();
    const unsigned long stageDurationMs =
        static_cast<unsigned long>(currentStage().durationSeconds) * 1000UL;

    if (now - stageStartMs_ >= stageDurationMs) {
      currentStageIndex_ = (currentStageIndex_ + 1) % STAGE_COUNT;
      stageStartMs_ = now;
      applyOutputs();
    }

    if (now - lastCountdownRefreshMs_ >= 100UL) {
      refreshCountdowns();
      lastCountdownRefreshMs_ = now;
    }
  }

 private:
  const StageDefinition& currentStage() const {
    return STAGES[currentStageIndex_];
  }

  LampColor colorForGroup(uint8_t stageIndex, GroupId group) const {
    return group == NS_GROUP ? STAGES[stageIndex].nsColor : STAGES[stageIndex].ewColor;
  }

  uint8_t secondsRemainingInCurrentStage() const {
    const unsigned long now = millis();
    const unsigned long elapsedMs = now - stageStartMs_;
    const unsigned long durationMs =
        static_cast<unsigned long>(currentStage().durationSeconds) * 1000UL;

    if (elapsedMs >= durationMs) {
      return 0;
    }

    return static_cast<uint8_t>((durationMs - elapsedMs + 999UL) / 1000UL);
  }

  uint8_t secondsUntilColorChange(GroupId group) const {
    const LampColor currentColor = colorForGroup(currentStageIndex_, group);
    uint16_t totalSeconds = secondsRemainingInCurrentStage();
    uint8_t stageIndex = (currentStageIndex_ + 1) % STAGE_COUNT;

    while (stageIndex != currentStageIndex_) {
      const LampColor nextColor = colorForGroup(stageIndex, group);
      if (nextColor != currentColor) {
        break;
      }

      totalSeconds += STAGES[stageIndex].durationSeconds;
      stageIndex = (stageIndex + 1) % STAGE_COUNT;
    }

    return totalSeconds > 99 ? 99 : static_cast<uint8_t>(totalSeconds);
  }

  void applyOutputs() const {
    const LampColor nsColor = currentStage().nsColor;
    const LampColor ewColor = currentStage().ewColor;

    northLight.setColor(nsColor);
    southLight.setColor(nsColor);
    eastLight.setColor(ewColor);
    westLight.setColor(ewColor);
  }

  void refreshCountdowns() {
    const uint8_t nsSeconds = secondsUntilColorChange(NS_GROUP);
    const uint8_t ewSeconds = secondsUntilColorChange(EW_GROUP);

    countdownScanner.setNumber(NORTH, nsSeconds);
    countdownScanner.setNumber(SOUTH, nsSeconds);
    countdownScanner.setNumber(EAST, ewSeconds);
    countdownScanner.setNumber(WEST, ewSeconds);
  }

  uint8_t currentStageIndex_ = 0;
  unsigned long stageStartMs_ = 0;
  unsigned long lastCountdownRefreshMs_ = 0;
};

IntersectionController controller;

void setup() {
  controller.begin();
}

void loop() {
  countdownScanner.service();
  controller.update();
}
