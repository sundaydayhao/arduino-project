#pragma once

#include <Arduino.h>

class ShiftRegisterBus {
 public:
  ShiftRegisterBus(uint8_t dataPin, uint8_t clockPin, uint8_t latchPin);

  void begin();
  void clear();
  void setBit(uint8_t bitIndex, bool level);
  bool getBit(uint8_t bitIndex) const;
  void writeMasked(uint32_t mask, uint32_t value);
  uint32_t raw() const;
  void flush();

 private:
  uint8_t dataPin_;
  uint8_t clockPin_;
  uint8_t latchPin_;
  uint32_t outputBits_;
};
