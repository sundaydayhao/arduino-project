#pragma once

#include <Arduino.h>

#include "ShiftRegisterBus.h"
#include "TrafficTypes.h"

class LampDriver {
 public:
  explicit LampDriver(ShiftRegisterBus& bus);

  void begin();
  void clear();
  void setLamp(Direction direction, LampColor color);
  void setBuzzer(bool enabled);
  void applySnapshot(const IntersectionSnapshot& snapshot);

 private:
  ShiftRegisterBus& bus_;
};
