#pragma once

#include <Arduino.h>

#include "TrafficTypes.h"

enum class Phase : uint8_t {
  NorthSouthGreen = 0,
  NorthSouthYellow = 1,
  AllRedToEastWest = 2,
  EastWestGreen = 3,
  EastWestYellow = 4,
  AllRedToNorthSouth = 5,
};

struct StageDefinition {
  Phase phase;
  uint32_t durationMs;
};

class IntersectionController {
 public:
  IntersectionController();

  void begin(uint32_t nowMs);
  bool update(uint32_t nowMs);
  const IntersectionSnapshot& snapshot() const;
  Phase phase() const;

 private:
  void advanceStage(uint32_t nowMs);
  void rebuildSnapshot(uint32_t nowMs);
  uint32_t remainingMs(uint32_t nowMs) const;
  uint8_t remainingSeconds(uint32_t milliseconds) const;
  void setDirectionState(Direction direction, LampColor lampColor, uint8_t seconds);

  static const StageDefinition kStages[6];

  uint8_t currentStageIndex_;
  uint32_t stageStartMs_;
  IntersectionSnapshot snapshot_;
};
