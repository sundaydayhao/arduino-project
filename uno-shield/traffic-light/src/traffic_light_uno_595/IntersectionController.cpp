#include "IntersectionController.h"

namespace {

constexpr uint32_t kNorthSouthGreenMs = 15000UL;
constexpr uint32_t kNorthSouthYellowMs = 3000UL;
constexpr uint32_t kAllRedToEastWestMs = 2000UL;
constexpr uint32_t kEastWestGreenMs = 15000UL;
constexpr uint32_t kEastWestYellowMs = 3000UL;
constexpr uint32_t kAllRedToNorthSouthMs = 2000UL;

}  // namespace

const StageDefinition IntersectionController::kStages[6] = {
  {Phase::NorthSouthGreen, kNorthSouthGreenMs},
  {Phase::NorthSouthYellow, kNorthSouthYellowMs},
  {Phase::AllRedToEastWest, kAllRedToEastWestMs},
  {Phase::EastWestGreen, kEastWestGreenMs},
  {Phase::EastWestYellow, kEastWestYellowMs},
  {Phase::AllRedToNorthSouth, kAllRedToNorthSouthMs},
};

IntersectionController::IntersectionController()
    : currentStageIndex_(0U),
      stageStartMs_(0UL),
      snapshot_{} {}

void IntersectionController::begin(uint32_t nowMs) {
  currentStageIndex_ = 0U;
  stageStartMs_ = nowMs;
  rebuildSnapshot(nowMs);
}

bool IntersectionController::update(uint32_t nowMs) {
  bool changed = false;

  while ((nowMs - stageStartMs_) >= kStages[currentStageIndex_].durationMs) {
    advanceStage(stageStartMs_ + kStages[currentStageIndex_].durationMs);
    changed = true;
  }

  rebuildSnapshot(nowMs);
  return changed;
}

const IntersectionSnapshot& IntersectionController::snapshot() const {
  return snapshot_;
}

Phase IntersectionController::phase() const {
  return kStages[currentStageIndex_].phase;
}

void IntersectionController::advanceStage(uint32_t nowMs) {
  currentStageIndex_ = (currentStageIndex_ + 1U) % 6U;
  stageStartMs_ = nowMs;
}

void IntersectionController::rebuildSnapshot(uint32_t nowMs) {
  const uint32_t stageRemainingMs = remainingMs(nowMs);
  const uint8_t stageRemainingSeconds = remainingSeconds(stageRemainingMs);

  snapshot_.buzzerEnabled = false;

  switch (phase()) {
    case Phase::NorthSouthGreen: {
      const uint8_t ewSeconds = remainingSeconds(stageRemainingMs + kNorthSouthYellowMs + kAllRedToEastWestMs);
      setDirectionState(Direction::North, LampColor::Green, stageRemainingSeconds);
      setDirectionState(Direction::South, LampColor::Green, stageRemainingSeconds);
      setDirectionState(Direction::East, LampColor::Red, ewSeconds);
      setDirectionState(Direction::West, LampColor::Red, ewSeconds);
      break;
    }

    case Phase::NorthSouthYellow: {
      const uint8_t ewSeconds = remainingSeconds(stageRemainingMs + kAllRedToEastWestMs);
      snapshot_.buzzerEnabled = true;
      setDirectionState(Direction::North, LampColor::Yellow, stageRemainingSeconds);
      setDirectionState(Direction::South, LampColor::Yellow, stageRemainingSeconds);
      setDirectionState(Direction::East, LampColor::Red, ewSeconds);
      setDirectionState(Direction::West, LampColor::Red, ewSeconds);
      break;
    }

    case Phase::AllRedToEastWest: {
      const uint8_t nsSeconds = remainingSeconds(
          stageRemainingMs + kEastWestGreenMs + kEastWestYellowMs + kAllRedToNorthSouthMs);
      setDirectionState(Direction::North, LampColor::Red, nsSeconds);
      setDirectionState(Direction::South, LampColor::Red, nsSeconds);
      setDirectionState(Direction::East, LampColor::Red, stageRemainingSeconds);
      setDirectionState(Direction::West, LampColor::Red, stageRemainingSeconds);
      break;
    }

    case Phase::EastWestGreen: {
      const uint8_t nsSeconds = remainingSeconds(stageRemainingMs + kEastWestYellowMs + kAllRedToNorthSouthMs);
      setDirectionState(Direction::North, LampColor::Red, nsSeconds);
      setDirectionState(Direction::South, LampColor::Red, nsSeconds);
      setDirectionState(Direction::East, LampColor::Green, stageRemainingSeconds);
      setDirectionState(Direction::West, LampColor::Green, stageRemainingSeconds);
      break;
    }

    case Phase::EastWestYellow: {
      const uint8_t nsSeconds = remainingSeconds(stageRemainingMs + kAllRedToNorthSouthMs);
      snapshot_.buzzerEnabled = true;
      setDirectionState(Direction::North, LampColor::Red, nsSeconds);
      setDirectionState(Direction::South, LampColor::Red, nsSeconds);
      setDirectionState(Direction::East, LampColor::Yellow, stageRemainingSeconds);
      setDirectionState(Direction::West, LampColor::Yellow, stageRemainingSeconds);
      break;
    }

    case Phase::AllRedToNorthSouth: {
      const uint8_t ewSeconds = remainingSeconds(
          stageRemainingMs + kNorthSouthGreenMs + kNorthSouthYellowMs + kAllRedToEastWestMs);
      setDirectionState(Direction::North, LampColor::Red, stageRemainingSeconds);
      setDirectionState(Direction::South, LampColor::Red, stageRemainingSeconds);
      setDirectionState(Direction::East, LampColor::Red, ewSeconds);
      setDirectionState(Direction::West, LampColor::Red, ewSeconds);
      break;
    }
  }
}

uint32_t IntersectionController::remainingMs(uint32_t nowMs) const {
  const uint32_t elapsed = nowMs - stageStartMs_;
  const uint32_t duration = kStages[currentStageIndex_].durationMs;
  return (elapsed >= duration) ? 0UL : (duration - elapsed);
}

uint8_t IntersectionController::remainingSeconds(uint32_t milliseconds) const {
  if (milliseconds == 0UL) {
    return 0U;
  }

  const uint32_t rounded = (milliseconds + 999UL) / 1000UL;
  return (rounded > 99UL) ? 99U : static_cast<uint8_t>(rounded);
}

void IntersectionController::setDirectionState(Direction direction, LampColor lampColor, uint8_t seconds) {
  const uint8_t index = toIndex(direction);
  snapshot_.lamps[index] = lampColor;
  snapshot_.countdownSeconds[index] = seconds;
}
