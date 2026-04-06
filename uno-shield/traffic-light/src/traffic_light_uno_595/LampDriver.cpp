#include "LampDriver.h"

#include "BoardConfig.h"

LampDriver::LampDriver(ShiftRegisterBus& bus)
    : bus_(bus) {}

void LampDriver::begin() {
  clear();
}

void LampDriver::clear() {
  for (uint8_t directionIndex = 0; directionIndex < kDirectionCount; ++directionIndex) {
    setLamp(static_cast<Direction>(directionIndex), LampColor::Off);
  }
  setBuzzer(false);
}

void LampDriver::setLamp(Direction direction, LampColor color) {
  const uint8_t index = toIndex(direction);

  bus_.setBit(BoardConfig::kLampRedBits[index], color == LampColor::Red);
  bus_.setBit(BoardConfig::kLampYellowBits[index], color == LampColor::Yellow);
  bus_.setBit(BoardConfig::kLampGreenBits[index], color == LampColor::Green);
}

void LampDriver::setBuzzer(bool enabled) {
  bus_.setBit(BoardConfig::kBuzzerBit, enabled);
}

void LampDriver::applySnapshot(const IntersectionSnapshot& snapshot) {
  for (uint8_t directionIndex = 0; directionIndex < kDirectionCount; ++directionIndex) {
    setLamp(static_cast<Direction>(directionIndex), snapshot.lamps[directionIndex]);
  }

  setBuzzer(snapshot.buzzerEnabled);
}
