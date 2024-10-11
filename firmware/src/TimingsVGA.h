//-*- C++ -*-
//
// Copyright (C) 2024 Scrap Computing
//

#ifndef __TIMINGSVGA_H__
#define __TIMINGSVGA_H__

#include <optional>
#include <cstdlib>

enum LineSection {
  H_FrontPorch = 0,
  H_Visible,
  H_BackPorch,
  H_Sync,
  V_FrontPorch = 4,
  V_Visible,
  V_BackPorch,
  V_Sync,
  H_SyncPolarity,
  V_SyncPolarity,
  H_Hz,
  V_Hz,
  PxClk,
  TTL,
};
enum Resolution {
  VGA_640x400_70Hz = 0,
  VGA_640x480_60Hz,
  VGA_800x600_56Hz,
  VGA_720x400_85Hz,
  MDA_720x350_50Hz,
  CGA_640x200_60Hz,
  EGA_640x200_60Hz,
  EGA_640x350_60Hz,
  RES_MAX,
};

static const char *modeToStr(Resolution M) {
  switch (M) {
  case VGA_640x400_70Hz:
    return "VGA 640x400@70Hz";
  case VGA_640x480_60Hz:
    return "VGA 640x480@60Hz";
  case VGA_800x600_56Hz:
    return "VGA 800x600@56Hz";
  case VGA_720x400_85Hz:
    return "VGA 720x400@85Hz";
  case MDA_720x350_50Hz:
    return "MDA 720x350@50Hz";
  case CGA_640x200_60Hz:
    return "CGA 640x200@60Hz";
  case EGA_640x200_60Hz:
    return "EGA 640x200@60Hz";
  case EGA_640x350_60Hz:
    return "EGA 640x350@60Hz";
  }
  return "UNKNOWN";
}

static constexpr const uint32_t UN = 99999; // Unknown values - to be filled in.
static constexpr const bool Pos = 0;
static constexpr const bool Neg = 1;
// Extra boundary because XBorder is not precise
static constexpr const int XB = 8;
static constexpr const uint32_t Timing[][14] = {
  // Front/back porches don't matter much.
  // What does matter is the pixel clock and HSync frequency, which gives us
  // the total number of "pixels" per line. We can get this by dividing
  // the Pixel Clock by the Horizontal Sync frequency.
  // MDA 720x350: 16257 / 18.43 = 882 "pixels"
  // CGA 640x200: 14318 / 15.7  = 912 "pixels"
  // EGA 640x350: 16257 / 21.85 = 744 "pixels"
  //
  // Note: F/B porch includes black borders.
  //                                                        Pos: ___|~|___
  //
  //                                                        Neg: ~~~|_|~~~
  //
  //                        Horizontal       Vertical       Polarity
  //                   ------------------ ---------------  ----------
  //                   F   Vis    B  Sync  F  Vis  B  Sync HSync VSync H_Hz V_Hz  PxClk TTL
  /*VGA_640x400@70:*/ {16, 640+XB, 48, 96-XB, 12, 400, 35,  2,  Neg,  Pos, 31469, 70, 25175000, 0},
  /*VGA_640x480@60:*/ {16, 640+XB, 48, 96-XB, 10, 480, 33,  2,  Neg,  Neg, 31469, 60, 25175000, 0},
  /*VGA_800x600@56:*/ {24, 800+XB,128, 72-XB,  1, 600, 22,  2,  Pos,  Pos, 35156, 56, 35156000, 0},
  /*VGA_720x400@85:*/ {36, 720+XB,108, 72-XB,  1, 400, 42,  3,  Neg,  Pos, 37927, 85, 35500000, 0},

  // NOTE: We need power of 2 F, Vis and B for 4-byte alignment!!!
  /*MDA_720x350@50:*/ {16, 720+XB, 20,   126, 20, 350,  4, 16,  Pos,  Neg, 18430, 50, 16257000, 1},
  /*CGA_640x200@60:*/ {68, 640+XB, 40,   163, 30, 200, 30,  3,  Pos,  Pos, 15700, 60, 14318181, 1},
  /*EGA_640x200@60:*/ {68, 640+XB, 40,   163, 20, 200, 40,  3,  Pos,  Pos, 15700, 60, 14318181, 1},
  /*EGA_640x350@60:*/ {28, 640+XB, 20,    56, 4,  350, 56,  3,  Pos,  Neg, 21850, 60, 16257000, 1},
};

static constexpr int HzError = 3;

static std::optional<Resolution> getModeForVPolarityAndHz(bool VSyncPolarity,
                                                          uint32_t ReqHz) {
  for (unsigned Res = 0; Res != RES_MAX; ++Res) {
    if (Timing[Res][TTL] != 1)
      continue;
    if (std::abs((int)Timing[Res][V_Hz] - (int)ReqHz) <= HzError) {
      if (Timing[Res][V_SyncPolarity] == VSyncPolarity)
        return (Resolution)Res;
    }
  }
  return std::nullopt;
}

#endif // __TIMINGSVGA_H__
