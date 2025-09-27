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

class DisplayBuffer {
  friend class XPM2;
  // The DMA used for copying text from TxtBuffer to Buffer.
  int DMAChannel;
  // The DMA used for filling in the frame buffer border with black.
  int DMAChannel2;

  const uint8_t *getCharSafe(char C) {
    // clang-format off
    switch(C) {
    case '0': return Char_0;
    case '1': return Char_1;
    case '2': return Char_2;
    case '3': return Char_3;
    case '4': return Char_4;
    case '5': return Char_5;
    case '6': return Char_6;
    case '7': return Char_7;
    case '8': return Char_8;
    case '9': return Char_9;
    case 'A': return Char_A;
    case 'B': return Char_B;
    case 'C': return Char_C;
    case 'D': return Char_D;
    case 'E': return Char_E;
    case 'F': return Char_F;
    case 'G': return Char_G;
    case 'H': return Char_H;
    case 'I': return Char_I;
    case 'J': return Char_J;
    case 'K': return Char_K;
    case 'L': return Char_L;
    case 'M': return Char_M;
    case 'N': return Char_N;
    case 'O': return Char_O;
    case 'P': return Char_P;
    case 'Q': return Char_Q;
    case 'R': return Char_R;
    case 'S': return Char_S;
    case 'T': return Char_T;
    case 'U': return Char_U;
    case 'V': return Char_V;
    case 'W': return Char_W;
    case 'X': return Char_X;
    case 'Y': return Char_Y;
    case 'Z': return Char_Z;
    case 'v': return Char_v;
    case 'x': return Char_x;
    case 'z': return Char_z;
    case '.': return Char_PERIOD;
    case ':': return Char_COLON;
    case '?': return Char_QUESTION;
    case '!': return Char_EXCLAMATION;
    case '@': return Char_AT;
    case ' ': return Char_SPACE;
    case '>': return Char_GT;
    case '+': return Char_PLUS;
    case '-': return Char_MINUS;
    case '[': return Char_SQBR_OPEN;
    case ']': return Char_SQBR_CLOSE;
    default: return Char_SPACE;
    }
    // clang-format on
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
  /// Writes Line to TxtBuffer.
  void displayTxt(const std::string &Line, int X, bool Center = false);
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
  /// Fi
  void fillBottomWithBlackAfter(uint32_t Line);
};

#endif // __BUFFER_H__
