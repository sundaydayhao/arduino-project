#pragma once

#include <Arduino.h>

#include "ShiftRegisterBus.h"
#include "TrafficTypes.h"

class DisplayScanner {
 public:
  explicit DisplayScanner(ShiftRegisterBus& bus);

  void begin();
  void clear();
  void setDigitGlyph(uint8_t digitIndex, int8_t glyph);
  void setNumber(Direction direction, uint8_t value, bool leadingZero = false);
  void service(uint32_t nowUs = micros());

 private:
  static const int8_t kGlyphBlank = -1;

  void clearSegments();
  void disableAllDigits();
  void selectDigit(uint8_t digitIndex);
  void applyGlyph(int8_t glyph);
  uint8_t encodeGlyph(int8_t glyph) const;

  ShiftRegisterBus& bus_;
  uint32_t lastScanUs_;
  uint8_t currentDigit_;
  int8_t glyphs_[8];
};
