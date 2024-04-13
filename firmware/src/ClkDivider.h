//-*- C++ -*-
//
// Copyright (C) 2024 Scrap Computing
//

#ifndef __CLKDIVIDER_H__
#define __CLKDIVIDER_H__

#include <limits>

/// A helper class for the PIO clock divider.
class ClkDivider {
  uint16_t DivInt;
  uint8_t DivFrac;

public:
  ClkDivider() : DivInt(1u), DivFrac(0u) {}
  ClkDivider(uint16_t Int, uint8_t Frac) : DivInt(Int), DivFrac(Frac) {}
  explicit ClkDivider(float Div)
      : DivInt((uint16_t)Div),
        DivFrac((uint8_t)((Div - (float)(uint16_t)Div) * (1u << 8u))) {}
  explicit ClkDivider(double Div)
      : DivInt((uint16_t)Div),
        DivFrac((uint8_t)((Div - (double)(uint16_t)Div) * (1u << 8u))) {}
  uint16_t getInt() const { return DivInt; }
  uint8_t getFrac() const { return DivFrac; }
  double get() const {
    return (double)DivInt + (double)DivFrac/ 256;
  }
  ClkDivider &operator++() {
    // Don't wrap
    if (DivInt != std::numeric_limits<decltype(DivInt)>::max() &&
        DivFrac == std::numeric_limits<decltype(DivFrac)>::max())
      return *this;
    ++DivFrac;
    if (DivFrac == 0u)
      ++DivInt;
    return *this;
  }
  ClkDivider &operator--() {
    // Don't wrap
    if (DivInt == 1u &&
        DivFrac == std::numeric_limits<decltype(DivFrac)>::min()) {
      return *this;
    }

    if (DivFrac == 0u)
      --DivInt;
    --DivFrac;
    return *this;
  }
  friend std::ostream &operator<<(std::ostream &OS, const ClkDivider &ClkDiv) {
    OS << "I" << (int)ClkDiv.getInt() << "F" << (int)ClkDiv.getFrac();
    return OS;
  }
};

#endif // __CLKDIVIDER_H__
