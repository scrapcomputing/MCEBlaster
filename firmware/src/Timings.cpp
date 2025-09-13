//-*- C++ -*-
//
// Copyright (C) 2024-2025 Scrap Computing
//

#include "Timings.h"
#include "Debug.h"
#include <iostream>

const char *modeToStr(VGAResolution R) {
  switch (R) {
    // clang-format off
#define DEF_VGA(NAME, ...) \
    case NAME: return #NAME;
#include "TimingsVGA.def"
    // clang-format on
  case VGA_MAX:
    return "VGA_MAX";
  }
  return "UNKNOWN";
}

int getTTLIdx(TTL M) {
  switch (M) {
  case TTL::MDA:
    return 0;
  case TTL::CGA:
    return 1;
  case TTL::EGA:
    return 2;
  }
  return -1;
}

TTL getTTLAtIdx(int Idx) {
  switch (Idx) {
  case 0:
    return TTL::MDA;
  case 1:
    return TTL::CGA;
  case 2:
    return TTL::EGA;
  }
  DBG_PRINT(std::cout << __FUNCTION__ << " Bad Idx\n";)
  return TTL::CGA;
}

const char *modeToStr(TTL M) {
  switch (M) {
  case TTL::MDA: return "MDA";
  case TTL::CGA: return "CGA";
  case TTL::EGA: return "EGA";
  }
  return "UNKNOWN";
}

const char *polarityToStr(Polarity P) {
  switch (P) {
  case Polarity::Neg:
    return "-";
  case Polarity::Pos:
    return "+";
  }
  return "ERR";
}

/// The allowed error for matching the measured Hz compared to the presets.
static constexpr float HzError = 1;

std::optional<TTLDescr> getModeForVPolarityAndHz(Polarity VSyncPolarity,
                                                 uint32_t ReqHz) {
  for (int Idx = 0; Idx != PresetTimingsMAX; ++Idx) {
    const auto &Mode = PresetTimingsTTL[Idx];
    if (std::abs(Mode.V_Hz - ReqHz) <= HzError) {
      if (Mode.V_SyncPolarity == VSyncPolarity)
        return Mode;
    }
  }
  return std::nullopt;
}

TTLDescr &TTLDescr::operator=(const TTLDescrReduced &Other) {
  Mode = Other.Mode;
  H_BackPorch = Other.H_BackPorch;
  H_Visible = Other.H_Visible;
  V_BackPorch = Other.V_BackPorch;
  V_Visible = Other.V_Visible;
  return *this;
}

void TTLDescr::dump(std::ostream &OS) const {
  OS << modeToStr(Mode) << " V:" << V_Hz << polarityToStr(V_SyncPolarity)
     << " H:" << H_Hz << polarityToStr(H_SyncPolarity) << " PxClk:" << PxClk;
}

void TTLDescr::dumpFull(std::ostream &OS, uint32_t SamplingOffset) const {
  char PxClkStr[10];
  snprintf(PxClkStr, 10, "%-2.3f", (float)PxClk / 1000000);
  char V_Hz_Str[10];
  snprintf(V_Hz_Str, 10, "%-2.3f", (float)V_Hz);
  char H_KHz_Str[10];
  snprintf(H_KHz_Str, 10, "%-2.3f", (float)H_Hz / 1000);

  OS << "VIDEO MODE: " << modeToStr(Mode) << "\n";
  OS << "VERTICAL SYNC:   " << V_Hz_Str
     << " Hz  POLARITY: " << polarityToStr(V_SyncPolarity) << "\n";
  OS << "HORIZONTAL SYNC: " << H_KHz_Str
     << " KHz POLARITY: " << polarityToStr(H_SyncPolarity) << "\n";
  OS << "PIXEL CLOCK: " << PxClkStr << "MHz SAMPLING OFFSET: " << SamplingOffset << "\n";
  OS << "HORIZONTAL VISIBLE:     " << H_Visible - XB << "\n";
  OS << "HORIZONTAL BACK PORCH: " << H_BackPorch << "\n";
  // OS << "HORIZONTAL FRONT PORCH:  " << H_FrontPorch << "\n";
  // OS << "HORIZONTAL RETRACE:     " << H_Sync << "\n";
  OS << "VERTICAL VISIBLE:     " << V_Visible - YB << "\n";
  OS << "VERTICAL BACK PORCH: " << V_BackPorch << "\n";
  // OS << "VERTICAL FRONT PORCH:  " << V_FrontPorch << "\n";
  // OS << "VERTICAL RETRACE:     " << V_Sync << "\n";
}

TTLDescrReduced &TTLDescrReduced::operator=(const TTLDescr &Other) {
  Mode = Other.Mode;
  H_BackPorch = Other.H_BackPorch;
  H_Visible = Other.H_Visible;
  V_BackPorch = Other.V_BackPorch;
  V_Visible = Other.V_Visible;
  return *this;
}

void TTLDescrReduced::dump(std::ostream &OS) const {
  OS << "VIDEO MODE: " << modeToStr(Mode) << "\n";
  OS << "HORIZONTAL VISIBLE:     " << H_Visible - XB << "\n";
  OS << "HORIZONTAL BACK PORCH: " << H_BackPorch << "\n";
  OS << "VERTICAL VISIBLE:     " << V_Visible - YB << "\n";
  OS << "VERTICAL BACK PORCH: " << V_BackPorch << "\n";
}
