#include "ShiftRegisterBus.h"

ShiftRegisterBus::ShiftRegisterBus(uint8_t dataPin, uint8_t clockPin, uint8_t latchPin)
    : dataPin_(dataPin),
      clockPin_(clockPin),
      latchPin_(latchPin),
      outputBits_(0UL) {}

void ShiftRegisterBus::begin() {
  pinMode(dataPin_, OUTPUT);
  pinMode(clockPin_, OUTPUT);
  pinMode(latchPin_, OUTPUT);

  digitalWrite(dataPin_, LOW);
  digitalWrite(clockPin_, LOW);
  digitalWrite(latchPin_, LOW);

  clear();
  flush();
}

void ShiftRegisterBus::clear() {
  outputBits_ = 0UL;
}

void ShiftRegisterBus::setBit(uint8_t bitIndex, bool level) {
  if (bitIndex >= 32) {
    return;
  }

  const uint32_t mask = (1UL << bitIndex);
  if (level) {
    outputBits_ |= mask;
  } else {
    outputBits_ &= ~mask;
  }
}

bool ShiftRegisterBus::getBit(uint8_t bitIndex) const {
  if (bitIndex >= 32) {
    return false;
  }

  return (outputBits_ & (1UL << bitIndex)) != 0;
}

void ShiftRegisterBus::writeMasked(uint32_t mask, uint32_t value) {
  outputBits_ = (outputBits_ & ~mask) | (value & mask);
}

uint32_t ShiftRegisterBus::raw() const {
  return outputBits_;
}

void ShiftRegisterBus::flush() {
  digitalWrite(latchPin_, LOW);

  for (int8_t byteIndex = 3; byteIndex >= 0; --byteIndex) {
    const uint8_t byteValue = static_cast<uint8_t>((outputBits_ >> (byteIndex * 8)) & 0xFFU);
    shiftOut(dataPin_, clockPin_, MSBFIRST, byteValue);
  }

  digitalWrite(latchPin_, HIGH);
  digitalWrite(latchPin_, LOW);
}
