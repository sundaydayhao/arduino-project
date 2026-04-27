#pragma once

#include <Arduino.h>

#include "TrafficTypes.h"

namespace BoardConfig {

constexpr uint8_t kShiftDataPin = 2;
constexpr uint8_t kShiftClockPin = 3;
constexpr uint8_t kShiftLatchPin = 4;
constexpr uint8_t kButtonPin = 6;
constexpr uint8_t kDebugLedPin = LED_BUILTIN;

constexpr uint8_t kShiftRegisterCount = 4;
constexpr uint8_t kOutputBitCount = 32;
constexpr uint8_t kSegmentCount = 8;
constexpr uint8_t kDigitCount = 8;
constexpr uint32_t kDisplayScanIntervalUs = 1500UL;
constexpr bool kDisplayDigitTestMode = false;
constexpr uint32_t kDisplayDigitTestHoldMs = 1000UL;

constexpr bool kSegmentOnLevel = true;
constexpr bool kDigitSelectActiveLow = true;

// Direction-indexed arrays follow the enum order:
// North, South, East, West.
// The assembled Uno shield routes the physical South and West groups in the
// opposite order, so those output groups are swapped here.
constexpr uint8_t kLampRedBits[kDirectionCount] = {
  0, 9, 6, 3
};

constexpr uint8_t kLampYellowBits[kDirectionCount] = {
  1, 10, 7, 4
};

constexpr uint8_t kLampGreenBits[kDirectionCount] = {
  2, 11, 8, 5
};

constexpr uint8_t kBuzzerBit = 12;

constexpr uint8_t kSegmentBits[kSegmentCount] = {
  16, 17, 18, 19, 20, 21, 22, 23
};

constexpr uint8_t kDigitBits[kDigitCount] = {
  24, 25, 26, 27, 28, 29, 30, 31
};

constexpr uint8_t kDirectionDigitBase[kDirectionCount] = {
  0, 6, 4, 2
};

static_assert(kOutputBitCount == kShiftRegisterCount * 8, "Unexpected shift-register width");

}  // namespace BoardConfig
