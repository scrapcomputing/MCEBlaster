//-*- C++ -*-
//
// Copyright (C) 2024-2025 Scrap Computing
//

#ifndef __TIMINGS_H__
#define __TIMINGS_H__

#include "Debug.h"
#include "Utils.h"
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <ostream>

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
};

enum VGAResolution {
// clang-format off
#define DEF_VGA(NAME, ...) \
  NAME,
#include "TimingsVGA.def"
// clang-format on
  VGA_MAX,
};

const char *modeToStr(VGAResolution M);

static constexpr const uint32_t UN = 99999; // Unknown values - to be filled in.

enum class Polarity {
  Pos,
  Neg,
};

static constexpr const Polarity Pos = Polarity::Pos;
static constexpr const Polarity Neg = Polarity::Neg;

const char polarityToChar(Polarity P);

// Extra boundary because XBorder is not precise.
// Ideally the buffer would contain only non-black pixels but our XBorder may be
// off by a few pixels, so we create a buffer XB larger to include XB black
// pixels.
#if defined(PICO_RP2040)
static constexpr const int XB = 8;
#elif defined(PICO_RP2350)
static constexpr const int XB = 16;
#endif
static constexpr const int YB = 2;

struct VGADescr {
  uint32_t H_FrontPorch;
  uint32_t H_Visible;
  uint32_t H_BackPorch;
  uint32_t H_Retrace;
  uint32_t V_FrontPorch;
  uint32_t V_Visible;
  uint32_t V_BackPorch;
  uint32_t V_Retrace;
  Polarity H_SyncPolarity;
  Polarity V_SyncPolarity;
  uint32_t H_Hz;
  uint32_t V_Hz;
  uint32_t PxClk;
};

static constexpr const VGADescr TimingsVGA[] = {
  // Front/back porches don't matter much.
  // What does matter is the pixel clock and HSync frequency, which gives us
  // the total number of "pixels" per line. We can get this by dividing
  // the Pixel Clock by the Horizontal Sync frequency.
  // MDA 720x350: 16257 / 18.43 = 882 "pixels"
  // CGA 640x200: 14318 / 15.7  = 912 "pixels"
  // EGA 640x350: 16257 / 21.85 = 744 "pixels"

  // clang-format off
#define DEF_VGA(NAME, ...) \
  {__VA_ARGS__},
#include "TimingsVGA.def"
// clang-format on
};

enum class TTL {
  MDA,
  CGA,
  EGA,
};
static constexpr const TTL MaxTTL = TTL::EGA;
static constexpr const int MaxTTLIdx = 2;

int getTTLIdx(TTL M);
TTL getTTLAtIdx(int Idx);

const char *modeToStr(TTL M);

struct TTLDescrReduced;

struct TTLDescr {
  uint32_t H_FrontPorch = 0;
  uint32_t H_Visible = 0;
  uint32_t H_BackPorch = 0;
  uint32_t H_Retrace = 0;
  uint32_t V_FrontPorch = 0;
  uint32_t V_Visible = 0;
  uint32_t V_BackPorch = 0;
  uint32_t V_Retrace = 0;
  Polarity H_SyncPolarity = Polarity::Pos;
  Polarity V_SyncPolarity = Polarity::Pos;
  float H_Hz = 0;
  float V_Hz = 0;
  uint32_t PxClk = 0;
  TTL Mode = TTL::CGA;
  TTLDescr &operator=(const TTLDescrReduced &Other);
  /// \Returns true if we don't need to switch Pio mode
  /// Porches/Retraces don't matter, they get adjusted on-the-fly
  /// Hz values are not actually used so skip them.
  /// PxClk is also skipped as it's not stored here
  /// Polaritis are also skipped as they get auto-detected.
  bool operator==(const TTLDescr &Other) const {
    return Mode == Other.Mode && H_Visible == Other.H_Visible &&
           V_Visible == Other.V_Visible;
  }
  bool operator!=(const TTLDescr &Other) const { return !(*this == Other); }
  void dump(Utils::StaticString<64> &SS) const;
  void dumpFull(Utils::StaticString<640> &Buff, uint32_t SamplingOffset) const;
  friend std::ostream &operator<<(std::ostream &OS, const TTLDescr &R) {
    Utils::StaticString<64> SS;
    R.dump(SS);
    OS << SS.get();
    return OS;
  }
};

/// This is used for storing the ManualTTL settings.
struct TTLDescrReduced {
  TTL Mode = TTL::CGA;
  int H_BackPorch = 0;
  uint32_t H_Visible = 0;
  int V_BackPorch = 0;
  uint32_t V_Visible = 0;
  TTLDescrReduced &operator=(const TTLDescr &Other);
  void dump(std::ostream &OS) const;
};

static constexpr const TTLDescr PresetTimingsTTL[] = {
// clang-format off
#define DEF_TTL(NAME, ...) \
  {__VA_ARGS__},
#include "TimingsTTL.def"
    // clang-format on
};

enum {
// clang-format off
#define DEF_TTL(NAME, ...) \
  NAME,
#include "TimingsTTL.def"
  // clang-format on
};

static constexpr const unsigned PresetTimingsMAX = 0
#define DEF_TTL(...) +1
#include "TimingsTTL.def"
    ;

std::optional<TTLDescr> getModeForVPolarityAndHz(Polarity VSyncPolarity,
                                                 uint32_t ReqHz);

static constexpr const int DisplayBufferDefaultTTL = 2; // EGA 640x200

#endif // __TIMINGS_H__
