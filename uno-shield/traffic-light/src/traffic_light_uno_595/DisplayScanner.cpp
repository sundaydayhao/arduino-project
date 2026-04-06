#include "DisplayScanner.h"

#include "BoardConfig.h"

DisplayScanner::DisplayScanner(ShiftRegisterBus& bus)
    : bus_(bus),
      lastScanUs_(0UL),
      currentDigit_(0U),
      glyphs_{kGlyphBlank, kGlyphBlank, kGlyphBlank, kGlyphBlank,
              kGlyphBlank, kGlyphBlank, kGlyphBlank, kGlyphBlank} {}

void DisplayScanner::begin() {
  clear();
  disableAllDigits();
  clearSegments();
  bus_.flush();
}

void DisplayScanner::clear() {
  for (uint8_t digitIndex = 0; digitIndex < BoardConfig::kDigitCount; ++digitIndex) {
    glyphs_[digitIndex] = kGlyphBlank;
  }
}

void DisplayScanner::setDigitGlyph(uint8_t digitIndex, int8_t glyph) {
  if (digitIndex >= BoardConfig::kDigitCount) {
    return;
  }

  glyphs_[digitIndex] = glyph;
}

void DisplayScanner::setNumber(Direction direction, uint8_t value, bool leadingZero) {
  const uint8_t base = BoardConfig::kDirectionDigitBase[toIndex(direction)];
  const uint8_t clamped = (value > 99U) ? 99U : value;
  const uint8_t tens = clamped / 10U;
  const uint8_t ones = clamped % 10U;

  glyphs_[base] = (tens == 0U && !leadingZero) ? kGlyphBlank : static_cast<int8_t>(tens);
  glyphs_[base + 1U] = static_cast<int8_t>(ones);
}

void DisplayScanner::service(uint32_t nowUs) {
  if ((nowUs - lastScanUs_) < BoardConfig::kDisplayScanIntervalUs) {
    return;
  }

  lastScanUs_ = nowUs;

  // Blank previous digit before changing segment data to reduce ghosting.
  disableAllDigits();
  clearSegments();
  bus_.flush();

  applyGlyph(glyphs_[currentDigit_]);
  selectDigit(currentDigit_);
  bus_.flush();

  currentDigit_ = (currentDigit_ + 1U) % BoardConfig::kDigitCount;
}

void DisplayScanner::clearSegments() {
  for (uint8_t segmentIndex = 0; segmentIndex < BoardConfig::kSegmentCount; ++segmentIndex) {
    bus_.setBit(BoardConfig::kSegmentBits[segmentIndex], !BoardConfig::kSegmentOnLevel);
  }
}

void DisplayScanner::disableAllDigits() {
  const bool inactiveLevel = BoardConfig::kDigitSelectActiveLow;
  for (uint8_t digitIndex = 0; digitIndex < BoardConfig::kDigitCount; ++digitIndex) {
    bus_.setBit(BoardConfig::kDigitBits[digitIndex], inactiveLevel);
  }
}

void DisplayScanner::selectDigit(uint8_t digitIndex) {
  if (digitIndex >= BoardConfig::kDigitCount) {
    return;
  }

  const bool activeLevel = !BoardConfig::kDigitSelectActiveLow;
  bus_.setBit(BoardConfig::kDigitBits[digitIndex], activeLevel);
}

void DisplayScanner::applyGlyph(int8_t glyph) {
  const uint8_t pattern = encodeGlyph(glyph);

  for (uint8_t segmentIndex = 0; segmentIndex < BoardConfig::kSegmentCount; ++segmentIndex) {
    const bool segmentOn = ((pattern >> segmentIndex) & 0x01U) != 0U;
    const bool busLevel = segmentOn ? BoardConfig::kSegmentOnLevel : !BoardConfig::kSegmentOnLevel;
    bus_.setBit(BoardConfig::kSegmentBits[segmentIndex], busLevel);
  }
}

uint8_t DisplayScanner::encodeGlyph(int8_t glyph) const {
  switch (glyph) {
    case 0: return 0b00111111;
    case 1: return 0b00000110;
    case 2: return 0b01011011;
    case 3: return 0b01001111;
    case 4: return 0b01100110;
    case 5: return 0b01101101;
    case 6: return 0b01111101;
    case 7: return 0b00000111;
    case 8: return 0b01111111;
    case 9: return 0b01101111;
    default: return 0b00000000;
  }
}
