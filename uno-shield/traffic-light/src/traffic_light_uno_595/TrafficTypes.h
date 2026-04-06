#pragma once

#include <Arduino.h>

enum class Direction : uint8_t {
  North = 0,
  South = 1,
  East = 2,
  West = 3,
};

enum class LampColor : uint8_t {
  Off = 0,
  Red = 1,
  Yellow = 2,
  Green = 3,
};

constexpr uint8_t kDirectionCount = 4;

inline uint8_t toIndex(Direction direction) {
  return static_cast<uint8_t>(direction);
}

struct IntersectionSnapshot {
  LampColor lamps[kDirectionCount];
  uint8_t countdownSeconds[kDirectionCount];
  bool buzzerEnabled;
};
