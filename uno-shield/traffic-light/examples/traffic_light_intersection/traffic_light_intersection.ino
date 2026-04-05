#include <Arduino.h>
#include <TM1637Display.h>

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

struct LightPins {
  uint8_t red;
  uint8_t yellow;
  uint8_t green;
};

struct DisplayPins {
  uint8_t clk;
  uint8_t dio;
};

struct StageDefinition {
  const char* name;
  uint8_t durationSeconds;
  LampColor nsColor;
  LampColor ewColor;
};

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

class CountdownModule {
 public:
  explicit CountdownModule(const DisplayPins& pins) : display_(pins.clk, pins.dio) {}

  void begin() {
    display_.setBrightness(0x0f, true);
    showNumber(0);
  }

  void showNumber(uint8_t seconds) {
    display_.showNumberDec(seconds, true, 2, 2);
  }

 private:
  TM1637Display display_;
};

static const LightPins LIGHT_PINS[DIRECTION_COUNT] = {
    {2, 3, 4},      // North
    {5, 6, 7},      // South
    {8, 9, 10},     // East
    {11, 12, 13},   // West
};

static const DisplayPins DISPLAY_PINS[DIRECTION_COUNT] = {
    {22, 23},   // North
    {24, 25},   // South
    {26, 27},   // East
    {28, 29},   // West
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

TrafficLightHead northLight(LIGHT_PINS[NORTH]);
TrafficLightHead southLight(LIGHT_PINS[SOUTH]);
TrafficLightHead eastLight(LIGHT_PINS[EAST]);
TrafficLightHead westLight(LIGHT_PINS[WEST]);

CountdownModule northDisplay(DISPLAY_PINS[NORTH]);
CountdownModule southDisplay(DISPLAY_PINS[SOUTH]);
CountdownModule eastDisplay(DISPLAY_PINS[EAST]);
CountdownModule westDisplay(DISPLAY_PINS[WEST]);

class IntersectionController {
 public:
  void begin() {
    northLight.begin();
    southLight.begin();
    eastLight.begin();
    westLight.begin();

    northDisplay.begin();
    southDisplay.begin();
    eastDisplay.begin();
    westDisplay.begin();

    currentStageIndex_ = 0;
    stageStartMs_ = millis();
    applyOutputs();
    refreshDisplays();
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

    if (now - lastRefreshMs_ >= 200UL) {
      refreshDisplays();
      lastRefreshMs_ = now;
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

  void refreshDisplays() const {
    const uint8_t nsSeconds = secondsUntilColorChange(NS_GROUP);
    const uint8_t ewSeconds = secondsUntilColorChange(EW_GROUP);

    northDisplay.showNumber(nsSeconds);
    southDisplay.showNumber(nsSeconds);
    eastDisplay.showNumber(ewSeconds);
    westDisplay.showNumber(ewSeconds);
  }

  uint8_t currentStageIndex_ = 0;
  unsigned long stageStartMs_ = 0;
  unsigned long lastRefreshMs_ = 0;
};

IntersectionController controller;

void setup() {
  controller.begin();
}

void loop() {
  controller.update();
}
