//-*- C++ -*-
//
// Copyright (C) 2024 Scarp Computing
//

#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "Bitmaps.h"
#include "Common.h"
#include "Debug.h"
#include "Timings.h"
#include "XPM2.h"
#include "hardware/dma.h"
#include <iostream>
#include <pico/stdlib.h>
#include <vector>

class DisplayBuffer {
  friend class XPM2;
  // The DMA used for copying text from TxtBuffer to Buffer.
  int DMAChannel;

  std::unordered_map<char, const uint8_t *> CharMap;

  const uint8_t *getCharSafe(char C) {
    auto It = CharMap.find(C);
    if (It == CharMap.end())
      return Char_SPACE;
    return It->second;
  }

public:
  static constexpr const uint32_t BuffX = 640 + XB;
  static constexpr const uint32_t BuffY = 350 + YB;

private:
  uint8_t Buffer[BuffY][BuffX] __attribute__((aligned(4)));

  XPM2 SplashXPM;

  static inline void setBit(uint8_t &Val, int BitN, int Bit) {
    Val ^= (-Bit ^ Val) & (1 << BitN);
  }
  /// This is linked to TTLReader's TTL mode.
  const TTLDescr *TimingsTTL = nullptr;

  static constexpr const int TxtBuffY = 8;
  static constexpr const int TxtBuffX = BuffX;
  uint8_t TxtBuffer[TxtBuffY][TxtBuffX] __attribute__((aligned(4)));

public:
  /// The top of the text, counting from the top of the buffer.
  uint32_t getTxtLineYTop() const { return TimingsTTL->V_Visible / 2; }
  /// The bottom of the text, counting from the top of the buffer.
  uint32_t getTxtLineYBot() const { return getTxtLineYTop() + TxtBuffY; }
  DisplayBuffer();
  void clear();
  void clearTxtBuffer();
  void noSignal();
  /// Writes to TxtBuffer.
  void setPixel(uint8_t Pixel, int X, int Y, uint8_t Buff[][BuffX]);
  /// Writes to TxtBuffer.
  void showBitmap(const uint8_t *BMap, int X, int Y, uint8_t Buff[][BuffX],
                  uint32_t FgColor, uint32_t BgColor, int ZoomXLevel = 1,
                  int ZoomYLevel = 1);
  void displayChar(char C, int X, int Y, uint8_t Buff[][BuffX],
                   uint32_t FgColor, uint32_t BgColor, int ZoomXLevel = 1,
                   int ZoomYLevel = 1);
  /// Writes Txt to TxtBuffer.
  void displayTxt(const std::string &Txt, int X, bool Center = false);
  /// Write a text page to the dipslay buffer (not TxtBuffer).
  void displayPage(const std::string &PageTxt, bool Center = true);
  /// Copy TxtBuffer to Buffer, i.e. show text on screen.
  void copyTxtBufferToScreen();

  inline void setBit(int Y, int X, int BitN, bool Val) {
    setBit(Buffer[Y][X], BitN, (int)Val);
  }
  inline void setMDA(int Y, int X, uint8_t RR) {
    int BitN = X % 2 == 0 ? 0 : 4;
    setBit(Y, X / 2, BitN + 1, (bool)(RR & 0x2));
    setBit(Y, X / 2, BitN, (bool)(RR & 0x1));
  }
  inline void setCGA(uint32_t Y, uint32_t X, uint8_t Val) {
    uint32_t LimitedX = std::min(TimingsTTL->H_Visible - 4, X);
    uint32_t LimitedY = std::min(TimingsTTL->V_Visible - 1, Y);
    Buffer[LimitedY][LimitedX] = Val;
  }
  inline void setCGA32(uint32_t Y, uint32_t X, uint32_t Val) {
    uint32_t LimitedX = std::min(BuffX - 4, X);
    uint32_t LimitedY = std::min(BuffY - 1, Y);
    (uint32_t &)Buffer[LimitedY][LimitedX] = Val;
  }
  inline void setMDA32(uint32_t Y, uint32_t X, uint32_t Val) {
    // XOffset to center image in 800x600 frame.
    const uint32_t XOffset =
        (TimingsVGA[VGA_800x600_56Hz].H_Visible - TimingsTTL->H_Visible) / 2;
    uint32_t LimitedX = std::min(BuffX - 4, X + XOffset);
    uint32_t LimitedY = std::min(BuffY - 1, Y);
    (uint32_t &)Buffer[LimitedY][LimitedX] = Val;
  }
  inline uint8_t getMDA(int Y, int X, int BitN) {
    if (Buffer[Y][X] & (1 << BitN))
      return Green;
    return Black;
  }
  /// \Returns 8 MDA pixels.
  inline uint32_t getMDA32(int Y, int X) {
    return (uint32_t &)Buffer[Y][X / 2];
  }
  inline uint8_t get(int Y, int X) { return Buffer[Y][X]; }
  inline uint32_t get32(int Y, int X) { return (uint32_t &)Buffer[Y][X]; }

  /// We only need to call this once.
  void setMode(const TTLDescr &NewMode) { TimingsTTL = &NewMode; }
};

#endif // __BUFFER_H__
