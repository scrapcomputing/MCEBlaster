//-*- C++ -*-
//
// Copyright (C) 2024 Scarp Computing
//

#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "Bitmaps.h"
#include "Common.h"
#include "Debug.h"
#include "TimingsVGA.h"
#include "XPM2.h"
#include <iostream>
#include <pico/stdlib.h>
#include <vector>

class DisplayBuffer {
  friend class XPM2;
  // Resolution Mode = CGA_640x200_60Hz;
  Resolution Mode = EGA_640x350_60Hz;
  // Resolution Mode = MDA_720x350_50Hz;

  std::unordered_map<char, const uint8_t *> CharMap;

  const uint8_t *getCharSafe(char C) {
    auto It = CharMap.find(C);
    if (It == CharMap.end())
      return Char_SPACE;
    return It->second;
  }

public:
  static constexpr const uint32_t BuffX = 640 + XB;
  static constexpr const uint32_t BuffY = 350;

private:
  uint8_t Buffer[BuffY][BuffX] __attribute__((aligned(4)));

  struct ModeData {
    int PixelClock_ns;
    int PixelClock_PIO;
  };

  ModeData Modes[RES_MAX];

  static inline void setBit(uint8_t &Val, int BitN, int Bit) {
    Val ^= (-Bit ^ Val) & (1 << BitN);
  }

public:
  DisplayBuffer();
  void clear();
  void noSignal();
  void setPixel(uint8_t Pixel, int X, int Y);
  void showBitmap(const uint8_t *BMap, int X, int Y, int ZoomXLevel = 1,
                  int ZoomYLevel = 1);
  /// Display \p TxtOrig at \p X and \p Y offsets from center.
  void displayTxt(const std::string &Txt, int X, int Y, int Zoom = 1,
                  bool Center = false);
  int getPixelClock_PIO() const { return Modes[Mode].PixelClock_PIO; }
  void setMode(Resolution NewMode) { Mode = NewMode; }
  inline Resolution getMode() const { return Mode; }

  inline void setBit(int Y, int X, int BitN, bool Val) {
    setBit(Buffer[Y][X], BitN, (int)Val);
  }
  inline void setMDA(int Y, int X, uint8_t RR) {
    int BitN = X % 2 == 0 ? 0 : 4;
    setBit(Y, X / 2, BitN + 1, (bool)(RR & 0x2));
    setBit(Y, X / 2, BitN, (bool)(RR & 0x1));
  }
  inline void setCGA(uint32_t Y, uint32_t X, uint8_t Val) {
    uint32_t LimitedX = std::min(Timing[CGA_640x200_60Hz][H_Visible] - 4, X);
    uint32_t LimitedY = std::min(Timing[CGA_640x200_60Hz][V_Visible] - 1, Y);
    Buffer[LimitedY][LimitedX] = Val;
  }
  inline void setCGA32(uint32_t Y, uint32_t X, uint32_t Val) {
    uint32_t LimitedX = std::min(BuffX - 4, X);
    uint32_t LimitedY = std::min(BuffY - 1, Y);
    (uint32_t &)Buffer[LimitedY][LimitedX] = Val;
  }
  inline void setMDA32(uint32_t Y, uint32_t X, uint32_t Val) {
    // XOffset to center image in 800x600 frame.
    static constexpr const uint32_t XOffset =
        (Timing[VGA_800x600_56Hz][H_Visible] -
         Timing[MDA_720x350_50Hz][H_Visible]) /
        2;
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

  static uint32_t getHz(uint32_t Period_us) {
    if (Period_us >= 19500 && Period_us <= 20500)
      return 50;
    else if (Period_us >= 17000 && Period_us <= 16300)
      return 60;
    return (uint32_t)((float)1000000 / Period_us);
  }

  static const char *getPolarityStr(bool Polarity) {
    if (Polarity == Neg)
      return "Neg";
    if (Polarity == Pos)
      return "Pos";
    return "ERR";
  }

  bool checkAndUpdateMode(int64_t FrameUs, uint32_t FrameCnt, bool VSyncPolarity) {
    int32_t Hz = DisplayBuffer::getHz(FrameUs);
    // DBG_PRINT(std::cout << "Frame us=" << FrameUs << " Hz=" << Hz
    //                     << " Polarity=" << getPolarityStr(VSyncPolarity)
    //                     << " mode=" << modeToStr(Mode) << "\n";)
    auto NewModeOpt = getModeForVPolarityAndHz(VSyncPolarity, Hz);
    if (!NewModeOpt) {
      if (FrameCnt % 1024 == 0)
        DBG_PRINT(std::cout << "Could not match Polarity="
                            << getPolarityStr(VSyncPolarity) << " and Hz=" << Hz
                            << "\n";)
      return false;
    }
    bool ChangeMode = *NewModeOpt != Mode;
    if (ChangeMode) {
      clear();
      setMode(*NewModeOpt);
      DBG_PRINT(std::cout << "\nUpdating Buff.Mode=" << modeToStr(Mode) << " Polarity="
                << getPolarityStr(VSyncPolarity) << " and Hz=" << Hz
                << "\n";)
    }
    return ChangeMode;
  }
};

#endif // __BUFFER_H__
