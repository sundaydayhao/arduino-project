#include <Arduino.h>

#include "BoardConfig.h"
#include "DisplayScanner.h"
#include "IntersectionController.h"
#include "LampDriver.h"
#include "ShiftRegisterBus.h"

namespace {

ShiftRegisterBus shiftBus(
    BoardConfig::kShiftDataPin,
    BoardConfig::kShiftClockPin,
    BoardConfig::kShiftLatchPin);
LampDriver lampDriver(shiftBus);
DisplayScanner displayScanner(shiftBus);
IntersectionController intersectionController;

const char* phaseName(Phase phase) {
  switch (phase) {
    case Phase::NorthSouthGreen: return "NS_GREEN";
    case Phase::NorthSouthYellow: return "NS_YELLOW";
    case Phase::AllRedToEastWest: return "ALL_RED_TO_EW";
    case Phase::EastWestGreen: return "EW_GREEN";
    case Phase::EastWestYellow: return "EW_YELLOW";
    case Phase::AllRedToNorthSouth: return "ALL_RED_TO_NS";
    default: return "UNKNOWN";
  }
}

void applySnapshot(const IntersectionSnapshot& snapshot) {
  lampDriver.applySnapshot(snapshot);

  displayScanner.setNumber(Direction::North, snapshot.countdownSeconds[toIndex(Direction::North)]);
  displayScanner.setNumber(Direction::East, snapshot.countdownSeconds[toIndex(Direction::East)]);
  displayScanner.setNumber(Direction::South, snapshot.countdownSeconds[toIndex(Direction::South)]);
  displayScanner.setNumber(Direction::West, snapshot.countdownSeconds[toIndex(Direction::West)]);
}

void printSnapshot(const IntersectionSnapshot& snapshot) {
  Serial.print("Phase=");
  Serial.print(phaseName(intersectionController.phase()));
  Serial.print(" | N=");
  Serial.print(snapshot.countdownSeconds[toIndex(Direction::North)]);
  Serial.print(" E=");
  Serial.print(snapshot.countdownSeconds[toIndex(Direction::East)]);
  Serial.print(" S=");
  Serial.print(snapshot.countdownSeconds[toIndex(Direction::South)]);
  Serial.print(" W=");
  Serial.print(snapshot.countdownSeconds[toIndex(Direction::West)]);
  Serial.print(" | Buzzer=");
  Serial.println(snapshot.buzzerEnabled ? "ON" : "OFF");
}

}  // namespace

void setup() {
  Serial.begin(115200);
  pinMode(BoardConfig::kDebugLedPin, OUTPUT);
  digitalWrite(BoardConfig::kDebugLedPin, LOW);

  shiftBus.begin();
  lampDriver.begin();
  displayScanner.begin();

  intersectionController.begin(millis());
  applySnapshot(intersectionController.snapshot());
  displayScanner.service(micros());

  Serial.println("Arduino Uno + 4x74HC595 traffic light skeleton");
  printSnapshot(intersectionController.snapshot());
}

void loop() {
  const uint32_t nowMs = millis();
  const bool stageChanged = intersectionController.update(nowMs);

  applySnapshot(intersectionController.snapshot());
  displayScanner.service(micros());

  digitalWrite(BoardConfig::kDebugLedPin, intersectionController.snapshot().buzzerEnabled ? HIGH : LOW);

  if (stageChanged) {
    printSnapshot(intersectionController.snapshot());
  }
}
