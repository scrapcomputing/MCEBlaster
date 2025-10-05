//-*- C++ -*-
//
// Copyright (C) 2024 Scrap Computing
//

#include "TTLReader.h"
#include "Common.h"
#include "DisplayBuffer.h"
#include "Utils.h"
#include "pico/stdlib.h"
#include <cmath>

static constexpr const uint32_t EGABorderCounter = 700;
static constexpr const uint32_t CGABorderCounter = 700;
static constexpr const uint32_t MDABorderCounter = 800;

static inline void EGA640x350PioConfig(PIO Pio, uint SM, uint Offset,
                                       uint RGB_GPIO, uint32_t HSYNC_GPIO,
                                       uint16_t ClkDivInt, uint8_t ClkDivFrac,
                                       uint32_t InstrDelay,
                                       uint32_t SamplingOffset,
                                       Polarity HSyncPolarity) {
  auto GetSmConfig = [Offset](uint32_t InstrDelay, uint32_t SamplingOffset,
                              Polarity HSyncPolarity) {
    switch (HSyncPolarity) {
    case Polarity::Pos: {
#include "EGASwitchCase_PosHSync_config_cpp"
      break;
    }
    case Polarity::Neg: {
#include "EGASwitchCase_NegHSync_config_cpp"
      break;
    }
    }
    std::cerr << "Bad InstrDelay or SamplingOffset in EGA: GetSmConfig("
              << InstrDelay << ", " << SamplingOffset << ")\n";
    exit(1);
  };
  pio_sm_config Conf = GetSmConfig(InstrDelay, SamplingOffset, HSyncPolarity);

  // in pins: RGB
  sm_config_set_in_pins(&Conf, RGB_GPIO);
  // Shift to the right, no auto-push
  sm_config_set_in_shift(&Conf, /*shift_right=*/true, /*autopush=*/false,
                         /*push_threshold=*/32);
  // We only need an input fifo, so create a 8-entry queue.
  sm_config_set_fifo_join(&Conf, PIO_FIFO_JOIN_RX);
  // Set the clock divider
  sm_config_set_clkdiv_int_frac(&Conf, ClkDivInt, ClkDivFrac);

  sm_config_set_jmp_pin(&Conf, HSYNC_GPIO);
  // Initializations
  // Set pin direction
  pio_sm_set_consecutive_pindirs(Pio, SM, RGB_GPIO, 8, /*is_out=*/false);

  pio_sm_init(Pio, SM, Offset, &Conf);
}

static inline void CGA640x200PioConfig(PIO Pio, uint SM, uint Offset,
                                       uint RGB_GPIO, uint32_t HSYNC_GPIO,
                                       uint16_t ClkDivInt, uint8_t ClkDivFrac,
                                       uint32_t InstrDelay,
                                       uint32_t SamplingOffset,
                                       Polarity HSyncPolarity) {
  auto GetSmConfig = [Offset](uint32_t InstrDelay, uint32_t SamplingOffset,
                              Polarity HSyncPolarity) {
    switch (HSyncPolarity) {
    case Polarity::Pos: {
#include "CGASwitchCase_PosHSync_config_cpp"
      break;
    }
    case Polarity::Neg: {
#include "CGASwitchCase_NegHSync_config_cpp"
    }
    }
    std::cerr << "Bad InstrDelay or SamplingOffset in CGA: GetSmConfig("
              << InstrDelay << ", " << SamplingOffset << ")\n";
    exit(1);
  };
  pio_sm_config Conf = GetSmConfig(InstrDelay, SamplingOffset, HSyncPolarity);
  // in pins: RGB
  sm_config_set_in_pins(&Conf, RGB_GPIO);
  // Shift to the right, no auto-push
  // This means that if we IN from Pins == 0b0011, then ISR = 0b00...0011
  sm_config_set_in_shift(&Conf, /*shift_right=*/true, /*autopush=*/false,
                         /*push_threshold=*/32);

  // Out: Shift Right
  sm_config_set_out_shift(&Conf, /*shift_right=*/true, /*autopush=*/false,
                          /*push_threshold=*/32);

  sm_config_set_jmp_pin(&Conf, HSYNC_GPIO);

  // We only need an input fifo, so create a 8-entry queue.
  sm_config_set_fifo_join(&Conf, PIO_FIFO_JOIN_RX);

  // Set the clock divider
  sm_config_set_clkdiv_int_frac(&Conf, ClkDivInt, ClkDivFrac);

  // Initializations
  // Set pin direction
  pio_sm_set_consecutive_pindirs(Pio, SM, RGB_GPIO, 8, /*is_out=*/false);

  pio_sm_init(Pio, SM, Offset, &Conf);
}

static inline void MDA720x350PioConfig(PIO Pio, uint SM, uint Offset,
                                       uint MDA_GPIO, uint HSYNC_GPIO,
                                       uint16_t ClkDivInt, uint8_t ClkDivFrac,
                                       uint32_t InstrDelay,
                                       uint32_t SamplingOffset,
                                       Polarity HSyncPolarity) {
  auto GetSmConfig = [Offset](uint32_t InstrDelay, uint32_t SamplingOffset, Polarity HSyncPolarity) {
    switch (HSyncPolarity) {
    case Polarity::Pos: {
#include "MDASwitchCase_PosHSync_config_cpp"
      break;
    }
    case Polarity::Neg: {
#include "MDASwitchCase_NegHSync_config_cpp"
      break;
    }
    }
    std::cerr << "Bad InstrDelay or SamplingOffset in MDA: GetSmConfig("
              << InstrDelay << ", " << SamplingOffset << ")\n";
    exit(1);
  };
  pio_sm_config Conf = GetSmConfig(InstrDelay, SamplingOffset, HSyncPolarity);
  // in pins: VI (Video, Intensity)
  sm_config_set_in_pins(&Conf, MDA_GPIO);
  // Shift to the right, no auto-push
  sm_config_set_in_shift(&Conf, /*shift_right=*/true, /*autopush=*/false,
                         /*push_threshold=*/32);
  sm_config_set_jmp_pin(&Conf, HSYNC_GPIO);

  // We only need an input fifo, so create a 8-entry queue.
  sm_config_set_fifo_join(&Conf, PIO_FIFO_JOIN_RX);

  // Set the clock divider
  sm_config_set_clkdiv_int_frac(&Conf, ClkDivInt, ClkDivFrac);

  // Initializations
  // Set pin direction
  pio_sm_set_consecutive_pindirs(Pio, SM, MDA_GPIO, 2, /*is_out=*/false);
  pio_sm_set_consecutive_pindirs(Pio, SM, HSYNC_GPIO, 1, /*is_out=*/false);

  pio_sm_init(Pio, SM, Offset, &Conf);
}

void AutoAdjustBorder::resetBorders() {
  TmpXBorder = XBorderInit;
  TmpYBorder = YBorderInit;
}

void AutoAdjustBorder::applyBorders(const TTLDescr &TimingsTTL) {
  // Clear screen if borders have changed to avoid artifacts.
  if (XBorder != TmpXBorder || YBorder != TmpYBorder)
    TTLR.getBuff().clear();

  // Update TTLReader's values.
  XBorder = TmpXBorder;
  YBorder = TmpYBorder;

  switch (TimingsTTL.Mode) {
  case TTL::CGA:
  case TTL::EGA:
    if (TTLReader::isHighRes(TimingsTTL))
      EGABorderOpt = BorderXY(XBorder, YBorder);
    else
      CGABorderOpt = BorderXY(XBorder, YBorder);
    break;
  case TTL::MDA:
    // We have enough frame-buffer size to not have to worry about the beginning
    // of the visible pixels (due to 2-pixels per byte scheme) so just use an
    // XBorder of 0.
    XBorder = 0;
    MDABorderOpt = BorderXY(XBorder, YBorder);
    break;
  }
}

void AutoAdjustBorder::setBorder(const BorderXY &XY) {
  if (ManualTTLEnabled && !XBorderAUTO)
    XBorder = ManualTTL.H_BackPorch;
  else
    XBorder = XY.X;

  if (ManualTTLEnabled && !YBorderAUTO)
    YBorder = ManualTTL.V_BackPorch;
  else
    YBorder = XY.Y;
}

void AutoAdjustBorder::forceStart() {
  if (Enabled == State::Off)
    Enabled = State::SingleON;
  FrameCnt = 0u;
  StartFrame = FrameCnt + AUTO_ADJUST_START_AFTER;
  StopFrame = FrameCnt + AUTO_ADJUST_DURATION;
}

void AutoAdjustBorder::runAutoAdjust() {
  DBG_PRINT(std::cout << "Auto Adjusting...\n";)
  TTLR.displayTxt("AUTO ADJUST", AUTO_ADJUST_DISPLAY_MS);
  forceStart();
}

bool AutoAdjustBorder::frameTick(const TTLDescr &TimingsTTL) {
  if (Enabled == State::Off)
    return false;
  if (++ThrottleCnt % AUTO_ADJUST_THROTTLE_CNT == 0)
    return false;
  ++FrameCnt;
  if (FrameCnt == StartFrame) {
    resetBorders();
  } else if (FrameCnt == StopFrame) {
    applyBorders(TimingsTTL);
    DBG_PRINT(std::cout << "Auto Adust: XBorder=" << XBorder
                        << " YBorder=" << YBorder << "\n";)
    switch (Enabled) {
    case State::SingleON:
      Enabled = State::Off;
      break;
    case State::Off:
      break;
    }
    return true;
  }
  return false;
}

void AutoAdjustBorder::collect(PIO Pio, uint32_t SM, uint32_t ModeBorderCounter,
                               uint32_t Line, TTL Mode) {
  // Get the border value from the dedicated PIO.
  uint32_t Raw = pio_sm_get_blocking(Pio, SM);
  if (Raw == 0xffffffff)
    Raw = 0;

  if (Enabled != State::Off) {
    uint32_t Border = ModeBorderCounter - Raw;
    Border &= 0xfffffffc; // Must be 4-byte aligned!
    TmpXBorder = std::min(TmpXBorder, Border);
    // If line is non-empty set YBorder.
    bool LineNonEmpty = Raw != 0;
    if (LineNonEmpty)
      // I think we need Line - 4 because Border.pio fills up the 4-size FIFO
      // during VSync.
      TmpYBorder = std::min(TmpYBorder, std::max((uint32_t)0u, Line - 4));
  }
}

uint32_t &TTLReader::getPxClkFor(const TTLDescr &Descr) {
  switch (Descr.Mode) {
  case TTL::CGA:
  case TTL::EGA:
    return isHighRes(Descr) ? EGAPxClk : CGAPxClk;
  case TTL::MDA:
    return MDAPxClk;
  default:
    std::cerr << __FUNCTION__ << " BAD Mode " << modeToStr(Descr.Mode) << "\n";
    exit(1);
  }
}

uint32_t &TTLReader::getSamplingOffsetFor(const TTLDescr &Descr) {
  switch (Descr.Mode) {
  case TTL::CGA:
    return CGASamplingOffset;
  case TTL::EGA:
    return EGASamplingOffset;
  case TTL::MDA:
    return MDASamplingOffset;
  default:
    std::cerr << __FUNCTION__ << " BAD Mode " << modeToStr(Descr.Mode) << "\n";
    exit(1);
  }
}

static uint32_t getSamplingOffsetMod(const TTL M) {
  switch (M) {
  case TTL::CGA:
    return CGAMaxSamplingOffset + 1;
  case TTL::EGA:
    return EGAMaxSamplingOffset + 1;
  case TTL::MDA:
    return MDAMaxSamplingOffset + 1;
  }
  assert(0 && "unreachable!");
}

void TTLReader::displayPxClk() {
  const uint32_t &PixelClock = getPxClkFor(TimingsTTL);
  const uint32_t &SamplingOffset = getSamplingOffsetFor(TimingsTTL);
  static constexpr const int BuffSz = /*Profile*/12 + 64;
  static char Txt[BuffSz];
  snprintf(Txt, BuffSz,
           "PROFILE: %lu PxCLK:%2.3fMHz  SAMPLING OFFSET:%lu  (%s %lux%lu)",
           *ProfileBankOpt, (float)PixelClock / 1000000, SamplingOffset,
           modeToStr(TimingsTTL.Mode),
           TimingsTTL.H_Visible -
               /*XB is an implementation detail, hide it from user*/ XB,
           TimingsTTL.V_Visible - YB);
  displayTxt(Txt, PX_CLK_TXT_DISPLAY_MS);
}

void TTLReader::changePxClk(bool Increase, bool SmallStep, bool OffsetStep) {
  /// \Returns the ClkDiv for the current mode.
  uint32_t &PixelClock = getPxClkFor(TimingsTTL);
  uint32_t &SamplingOffset = getSamplingOffsetFor(TimingsTTL);
  const uint32_t SamplingOffsetMod = getSamplingOffsetMod(TimingsTTL.Mode);
  DBG_PRINT(std::cout << "\n-----------\n";)
  DBG_PRINT(std::cout << "Pixel Clock Before=" << PixelClock;)
  if (Increase) {
    DBG_PRINT(std::cout << " ++ ";)
    if (OffsetStep) {
      SamplingOffset = (SamplingOffset + 1) % SamplingOffsetMod;
      if (SamplingOffset == 0)
        PixelClock += PXL_CLK_SMALL_STEP;
    } else {
      PixelClock += SmallStep ? PXL_CLK_SMALL_STEP : PXL_CLK_STEP;
    }
  } else {
    DBG_PRINT(std::cout << " -- ";)
    if (OffsetStep) {
      SamplingOffset =
          SamplingOffset == 0 ? (SamplingOffsetMod - 1) : SamplingOffset - 1;
      if (SamplingOffset == SamplingOffsetMod - 1)
        PixelClock -= PXL_CLK_SMALL_STEP;
    } else {
      PixelClock -= SmallStep ? PXL_CLK_SMALL_STEP : PXL_CLK_STEP;
    }
  }
  DBG_PRINT(std::cout << "After=" << PixelClock << "\n";)
  DBG_PRINT(std::cout << "-----------\n\n";)
  displayPxClk();
}

void TTLReader::unclaimUsedSMs() {
  for (auto [Pio, SM] : UsedSMs) {
    pio_sm_set_enabled(Pio, SM, false);
    // Unclaim it.
    pio_sm_unclaim(Pio, SM);
  }
}

int TTLReader::claimUnusedSMSafe(PIO Pio) {
  int SM = pio_claim_unused_sm(TTLPio, true);
  UsedSMs.push_back({Pio, SM});
  return SM;
}

void TTLReader::updateSyncPolarityVariables() {
  auto GetPolarity = [this](PIO Pio, uint SM) {
    uint32_t Samples = pio_sm_get_blocking(Pio, SM);
    // DBG_PRINT(printf("GetPolarity: 0x%08x\n", Samples);)
    // Count the number of '1's. If more than 16 then it's negative ~~~~|_|~~~~
    uint32_t CntOnes = 0;
    for (int Idx = 0; Idx != 32; ++Idx) {
      if ((Samples >> Idx) & 0x1)
        ++CntOnes;
      if (CntOnes > 16)
        return Polarity::Neg;
    }
    return Polarity::Pos;
  };

  if (!pio_sm_is_rx_fifo_empty(VSyncPolarityPio, VSyncPolaritySM)) {
    // VSync
    Polarity NewVSyncPolarity = GetPolarity(VSyncPolarityPio, VSyncPolaritySM);
    DBG_PRINT(if (NewVSyncPolarity != VSyncPolarity) {
      std::cout << "NewVSyncPolarity=" << polarityToChar(NewVSyncPolarity)
                << "\n";
    })
    VSyncPolarity = NewVSyncPolarity;
  } else {
    DBG_PRINT(std::cerr << "VSync Polarity Pio FIFO empty!\n";)
  }

  if (!pio_sm_is_rx_fifo_empty(HSyncPolarityPio, HSyncPolaritySM)) {
    // HSync
    Polarity NewHSyncPolarity = GetPolarity(HSyncPolarityPio, HSyncPolaritySM);
    DBG_PRINT(if (NewHSyncPolarity != HSyncPolarity) {
      std::cout << "NewHSyncPolarity=" << polarityToChar(NewHSyncPolarity)
                << "\n";
    })
    HSyncPolarity = NewHSyncPolarity;
  } else {
    DBG_PRINT(std::cerr << "HSync Polarity Pio FIFO empty!\n";)
  }
}

void TTLReader::readConfigFromFlash() {
  DBG_PRINT(std::cout << "Reading from flash...\n";)
  CGAPxClk = (uint32_t)Flash.read(get(Profile::CGAPxClkIdx));
  EGAPxClk = (uint32_t)Flash.read(get(Profile::EGAPxClkIdx));
  MDAPxClk = (uint32_t)Flash.read(get(Profile::MDAPxClkIdx));

  EGASamplingOffset = (uint32_t)Flash.read(get(Profile::EGASamplingOffsetIdx));
  CGASamplingOffset = (uint32_t)Flash.read(get(Profile::CGASamplingOffsetIdx));
  MDASamplingOffset = (uint32_t)Flash.read(get(Profile::MDASamplingOffsetIdx));

  ManualTTLEnabled = (bool)Flash.read(get(Profile::ManualTTL_EnabledIdx));
  ManualTTL.Mode =
      getTTLAtIdx((uint32_t)Flash.read(get(Profile::ManualTTL_ModeIdx)));
  ManualTTL.H_Visible =
      (uint32_t)Flash.read(get(Profile::ManualTTL_H_VisibleIdx));
  ManualTTL.V_Visible =
      (uint32_t)Flash.read(get(Profile::ManualTTL_V_VisibleIdx));
  if (ManualTTLEnabled) {
    XBorderAUTO = (uint32_t)Flash.read(get(Profile::XBorderAUTOIdx));
    ManualTTL.H_BackPorch =
        (int)Flash.read(get(Profile::ManualTTL_H_BackPorchIdx));
    YBorderAUTO = (uint32_t)Flash.read(get(Profile::YBorderAUTOIdx));
    ManualTTL.V_BackPorch =
        (int)Flash.read(get(Profile::ManualTTL_V_BackPorchIdx));
  }

  auto ReadBorderSafe = [this](Profile Idx) -> std::optional<BorderXY> {
    uint32_t XY = (uint32_t)Flash.read(get(Idx));
    // If 0xFFFFFFFF then it's not valid
    if (XY == InvalidBorder)
      return std::nullopt;
    return BorderXY(XY);
  };

  CGABorderOpt = ReadBorderSafe(Profile::CGABorderIdx);
  EGABorderOpt = ReadBorderSafe(Profile::EGABorderIdx);
  MDABorderOpt = ReadBorderSafe(Profile::MDABorderIdx);

  DBG_PRINT(std::cout << "CGABorderOpt=" << (CGABorderOpt ? "Yes" : "No")
                      << "\n";)
  DBG_PRINT(std::cout << "EGABorderOpt=" << (EGABorderOpt ? "Yes" : "No")
                      << "\n";)
  DBG_PRINT(std::cout << "MDABorderOpt=" << (MDABorderOpt ? "Yes" : "No")
                      << "\n";)

  DBG_PRINT(std::cout << "CGAPxClk=" << CGAPxClk << "\n";)
  DBG_PRINT(std::cout << "EGAPxClk=" << EGAPxClk << "\n";)
  DBG_PRINT(std::cout << "MDAPxClk=" << MDAPxClk << "\n";)
  DBG_PRINT(std::cout << "ManualTTL Enabled=" << ManualTTLEnabled << "\n";)
  DBG_PRINT(std::cout << "ManualTTL Mode=" << modeToStr(ManualTTL.Mode)
                      << "\n";)
  DBG_PRINT(std::cout << "ManualTTL H Visible=" << ManualTTL.H_Visible << "\n";)
  DBG_PRINT(std::cout << "ManualTTL V Visible=" << ManualTTL.V_Visible << "\n";)
  DBG_PRINT(std::cout << "XBorderAUTO=" << XBorderAUTO << "\n";)
  DBG_PRINT(std::cout << "ManualTTL H BackPorch=" << ManualTTL.H_BackPorch
                      << "\n";)
  DBG_PRINT(std::cout << "YBorderAUTO=" << YBorderAUTO << "\n";)
  DBG_PRINT(std::cout << "ManualTTL V BackPorch=" << ManualTTL.V_BackPorch
                      << "\n";)
}

TTLReader::TTLReader(PioProgramLoader &PioLoader, Pico &Pi, FlashStorage &Flash,
                     DisplayBuffer &Buff, PIO VSyncPolarityPio,
                     uint VSyncPolaritySM, PIO HSyncPolarityPio,
                     uint HSyncPolaritySM, bool ResetToDefaults)
    : Pi(Pi), PioLoader(PioLoader),
      AutoAdjustBtn(AUTO_ADJUST_GPIO, Pi, "AutoAdjust"),
      PxClkBtn(PX_CLK_BTN_GPIO, Pi, "PxClk"),
      AutoAdjust(ManualTTLEnabled, ManualTTL, XBorderAUTO, XBorder, YBorderAUTO,
                 YBorder, CGABorderOpt, EGABorderOpt, MDABorderOpt, Flash,
                 *this),
      Flash(Flash), VSyncPolarityPio(VSyncPolarityPio),
      VSyncPolaritySM(VSyncPolaritySM), HSyncPolarityPio(HSyncPolarityPio),
      HSyncPolaritySM(HSyncPolaritySM), ResetToDefaults(ResetToDefaults),
      Buff(Buff), ManualTTLMenu(*this) {
  DBG_PRINT(std::cout << "\n\n\n\n\nTTLReader constructor start\n";)

  XBorder = 0;
  YBorder = 0;
  VHz = 0;
  HHz = 0;
  VSyncPolarity = Polarity::Pos;
  HSyncPolarity = Polarity::Pos;
  ProfileBankOpt = 0;

  Buff.setMode(TimingsTTL);
  if (ResetToDefaults) {
    DBG_PRINT(std::cout << "\n\n\n*** Reset to defaults ***\n\n\n";)
    saveToFlash(/*OnlyProfileBank=*/false, /*AllProfiles=*/true);
    // Flash the LED to let the user know that reset was succesfull
    for (uint32_t Cnt = 0; Cnt != 20; ++Cnt) {
      if (Cnt % 2 == 0)
        Pi.ledON();
      else
        Pi.ledOFF();
      Utils::sleep_ms(100);
    }
    Pi.ledON();
  } else {
    if (Flash.valid()) {
      ProfileBankOpt = (uint32_t)Flash.read(ProfileBankIdx);
      readConfigFromFlash();
    } else {
      DBG_PRINT(std::cout << "Flash not valid!\n";);
    }
  }
  if (ManualTTLEnabled)
    TimingsTTL = ManualTTL;

  TTLPio = pio0;
  TTLSM = claimUnusedSMSafe(TTLPio);

  TTLBorderPio = pio0;
  TTLBorderSM = claimUnusedSMSafe(TTLBorderPio);

  DBG_PRINT(std::cout << "TTLReader constructor getDividerAutomatically()\n";)
  getDividerAutomatically();
  DBG_PRINT(std::cout << "TTLReader constructor switchPio()\n";)
  switchPio();
  DBG_PRINT(std::cout << "TTLReader constructor end\n";)
}

template <bool DiscardData>
bool __not_in_flash_func(TTLReader::readLineCGA)(uint32_t Line) {
  // Now fill in the line until HSync is high.
  uint32_t X = 0;

  uint32_t XBorderAdj =
      (XBorder + /*FIFO sz=*/8 * /*Pixels per FIFO Entry=*/4) &
      0xfffffffc; // Must be 4-byte aligned!
  // Wait here if we are still in HSync retrace
  while (gpio_get(TTL_HSYNC_GPIO) != 0)
    ;
  if (Line < YBorder) {
    while (gpio_get(TTL_HSYNC_GPIO) == 0)
      ;
  } else {
    uint32_t XMax = TimingsTTL.H_Visible + XBorderAdj;
    while (true) {
      // Example:
      // ISR Values are right-shifted. 0 is the earliest, 3 is the latest
      //              3         2         1         0
      //         |---------|---------|---------|---------|
      // VHRGB0 = VHRR GGBB VHRR GGBB VHRR GGBB VHRR GGBB
      //
      // Pico is little endian, so low-order bits of a 32-bit int come in lower
      // addresses in memory. So when we write into Buff we need to write bytes
      // in order: 0, 1, 2, 3

      // Skip non-visible parts
      uint32_t VHRGB = pio_sm_get_blocking(TTLPio, TTLSM);
      if constexpr (!DiscardData) {
        if (X >= XBorderAdj) {
          Buff.setCGA32(Line - YBorder, X - XBorderAdj, VHRGB & RGBMask_4);
        }
      }
      X += 4;
      if (X > XMax) {
        break;
      }
    }
  }
  // Wait for HSYNC
  while (gpio_get(TTL_HSYNC_GPIO) == 0)
    ;

  // // Flush FIFO so that the remaining entries are not used by the next line
  // while(!pio_sm_is_rx_fifo_empty(TTLPio, TTLSM))
  //   pio_sm_get(TTLPio, TTLSM);
  bool InRetrace =
      gpio_get(TTL_VSYNC_GPIO) == (TimingsTTL.V_SyncPolarity == Pos);
  return InRetrace;
}

template <bool DiscardData>
bool __not_in_flash_func(TTLReader::readLineMDA)(uint32_t Line) {
  // Now fill in the line until HSync is high.
  uint32_t PixelX = 0;
  uint32_t BuffX = 0;
  uint32_t XBorderAdj =
      (XBorder + /*FIFO sz (not filling up)=*/4 * /*Pixels per FIFO Entry=*/8) &
      0xfffffffc; // Must be 4-byte aligned!

  // Wait here if we are still in HSync retrace
  while (gpio_get(TTL_HSYNC_GPIO) != 0)
    ;
  if (Line < YBorder) {
    while (gpio_get(TTL_HSYNC_GPIO) == 0)
      ;
  } else {
    uint32_t XMax = TimingsTTL.H_Visible;
    uint32_t XMaxPixelX = XMax + XBorderAdj;
    while (true) {
      // Example:
      // ISR Values are right-shifted. 0 is the earliest, 7 is the latest
      //              3         2         1         0
      //         |---------|---------|---------|---------|
      // MDA8 =   00VI 00VI 00VI 00VI 00VI 00VI 00VI 00VI
      //
      // So the natural way of inserting values to the ISR is with right-shift.
      // We need to come up with the order: 7 6 5 4 3 2 1 0
      uint32_t MDA8 = pio_sm_get_blocking(TTLPio, TTLSM);
      if constexpr (!DiscardData) {
        if (BuffX >= XBorderAdj)
          Buff.setMDA32(Line - YBorder, BuffX - XBorderAdj, MDA8);
      }
      // Since we are packing 2 monochrome values per byte we are increasing
      // BuffX by 4 instead of 8 to avoid dividing it by 2 again in setMDA32().
      BuffX += 4;
      PixelX += 8;
      if (PixelX > XMaxPixelX) {
        break;
      }
    }
  }
  // Wait for HSYNC
  while (gpio_get(TTL_HSYNC_GPIO) == 0)
    ;
  bool InRetrace =
      gpio_get(TTL_VSYNC_GPIO) == (TimingsTTL.V_SyncPolarity == Pos);
  return InRetrace;
}

static auto getEGAProgram(uint32_t InstrDelay, uint32_t SamplingOffset, Polarity HSync) {
  switch (HSync) {
  case Polarity::Pos: {
#include "EGASwitchCase_PosHSync_program_cpp"
    break;
  }
  case Polarity::Neg: {
#include "EGASwitchCase_NegHSync_program_cpp"
    break;
  }
  }
  std::cerr << "Bad InstrDelay or SamplingOffset in  getEGAProgram("
            << InstrDelay << "," << SamplingOffset << ")\n";
  exit(1);
}

static auto getCGAProgram(uint32_t InstrDelay, uint32_t SamplingOffset,
                          Polarity HSync) {
  switch (HSync) {
  case Polarity::Pos: {
#include "CGASwitchCase_PosHSync_program_cpp"
    break;
  }
  case Polarity::Neg: {
#include "CGASwitchCase_NegHSync_program_cpp"
    break;
  }
  }
  std::cerr << "Bad InstrDelay or SamplingOffset in getCGAProgram("
            << InstrDelay << ", " << SamplingOffset << ")\n";
  exit(1);
}

static auto getMDAProgram(uint32_t InstrDelay, uint32_t SamplingOffset, Polarity HSync) {
  switch (HSync) {
  case Polarity::Pos: {
#include "MDASwitchCase_PosHSync_program_cpp"
    break;
  }
  case Polarity::Neg: {
#include "MDASwitchCase_NegHSync_program_cpp"
    break;
  }
  }
  std::cerr << "Bad InstrDelay or SamplingOffset in getMDAProgram("
            << InstrDelay << ", " << SamplingOffset << ")\n";
  exit(1);
}

static std::pair<uint32_t, uint32_t> getIPPRange(TTL M) {
  switch (M) {
  case TTL::CGA:
  case TTL::EGA:
    return {4, 16};
  case TTL::MDA:
    return {5, 16};
  }
  std::cerr << "Bad Mode in getIPPRange(" << modeToStr(M) << ")\n";
  exit(1);
}

bool TTLReader::haveBorderFromFlash() const {
  switch (TimingsTTL.Mode) {
  case TTL::CGA:
  case TTL::EGA:
    return isHighRes(TimingsTTL) ? (bool)EGABorderOpt : (bool)CGABorderOpt;
  case TTL::MDA:
    return (bool)MDABorderOpt;
  }
  return false;
}

void TTLReader::switchPio() {
  DBG_PRINT(std::cout << "unloadAllPio()\n";)
  PioLoader.unloadAllPio(TTLPio, {TTLSM, TTLBorderSM});
  DBG_PRINT(std::cout << "\nTTLReader Switching PIO to "
                      << modeToStr(TimingsTTL.Mode) << "\n\n";)

  auto GetBorderIPP = [](TTL M) { return 10; };

  double PicoClk_Hz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS) * 1000;
  float BorderClkDiv =
      PicoClk_Hz / (getPxClkFor(TimingsTTL) * GetBorderIPP(TimingsTTL.Mode));
  DBG_PRINT(std::cout << "BORDER CLKDIV=" << BorderClkDiv << "\n";)

  switch (TimingsTTL.Mode) {
  case TTL::MDA: {
    TTLOffset = PioLoader.loadPIOProgram(
        TTLPio, TTLSM, getMDAProgram(MDAIPP, MDASamplingOffset, HSyncPolarity),
        [this](PIO Pio, uint SM, uint Offset) {
          MDA720x350PioConfig(Pio, SM, Offset, MDA_VI_GPIO, TTL_HSYNC_GPIO,
                              MDAClkDiv.getInt(), MDAClkDiv.getFrac(), MDAIPP,
                              MDASamplingOffset, HSyncPolarity);
        });

    TTLBorderOffset = PioLoader.loadPIOProgram(
        TTLBorderPio, TTLBorderSM, &MDA720x350Border_program,
        [BorderClkDiv](PIO Pio, uint SM, uint Offset) {
          MDA720x350BorderPioConfig(Pio, SM, Offset, MDA_VI_GPIO,
                                    TTL_HSYNC_GPIO, BorderClkDiv);
        });
    pio_sm_put_blocking(TTLBorderPio, TTLBorderSM,
                        /*Counter=*/MDABorderCounter);
    break;
  }
  case TTL::CGA:
  case TTL::EGA: {
    if (isHighRes(TimingsTTL)) {
      TTLOffset = PioLoader.loadPIOProgram(
          TTLPio, TTLSM,
          getEGAProgram(EGAIPP, EGASamplingOffset, HSyncPolarity),
          [this](PIO Pio, uint SM, uint Offset) {
            EGA640x350PioConfig(Pio, SM, Offset, EGA_RGB_GPIO, TTL_HSYNC_GPIO,
                                EGAClkDiv.getInt(), EGAClkDiv.getFrac(), EGAIPP,
                                EGASamplingOffset, HSyncPolarity);
          });

      TTLBorderOffset = PioLoader.loadPIOProgram(
          TTLBorderPio, TTLBorderSM, &EGA640x350Border_program,
          [BorderClkDiv](PIO Pio, uint SM, uint Offset) {
            EGA640x350BorderPioConfig(Pio, SM, Offset, EGA_RGB_GPIO,
                                      BorderClkDiv);
          });
      pio_sm_put_blocking(TTLBorderPio, TTLBorderSM,
                          /*Counter=*/EGABorderCounter);

    } else {
      TTLOffset = PioLoader.loadPIOProgram(
          TTLPio, TTLSM,
          getCGAProgram(CGAIPP, CGASamplingOffset, HSyncPolarity),
          [this](PIO Pio, uint SM, uint Offset) {
            CGA640x200PioConfig(Pio, SM, Offset, CGA_ACTUAL_RGB_GPIO,
                                TTL_HSYNC_GPIO, CGAClkDiv.getInt(),
                                CGAClkDiv.getFrac(), CGAIPP, CGASamplingOffset,
                                HSyncPolarity);
          });

      TTLBorderOffset = PioLoader.loadPIOProgram(
          TTLBorderPio, TTLBorderSM, &CGA640x200Border_program,
          [BorderClkDiv](PIO Pio, uint SM, uint Offset) {
            CGA640x200BorderPioConfig(Pio, SM, Offset, CGA_ACTUAL_RGB_GPIO,
                                      BorderClkDiv);
          });
      pio_sm_put_blocking(TTLBorderPio, TTLBorderSM,
                          /*Counter=*/CGABorderCounter);
    }
    break;
  }
  }

  if (!haveBorderFromFlash()) {
    DBG_PRINT(std::cout << "No borders from flash! AutoAdjust.forceStart()\n";)
    AutoAdjust.forceStart();
  }
  Buff.clear();
}

void TTLReader::getDividerAutomatically() {
  // Automatic tuning of sampling frequency
  // ---------------------------------------
  // The pixel frequency is standardized and stable across cards, so our goal
  // is to match the Pico's PIO pixel sampling with it.
  // This is not as easy as it seems for three reasons:
  // (i)   The Pico's frequency is kind-of fixed due to performance constraints.
  // (ii)  The PIO instrucitons can be delayed by a fixed integer amount which
  //       does not give us enough precision to match the pixel clock.
  // (iii) The PIO frequency dividier's fractional part has an 8-bit (i.e., 256
  //       positions) granularity, which is still not fine-grain enough.
  //
  // So to match the PIO sampling frequency as close as possible to the TTL
  // pixel frequency we need to minimize the error between the PIO sampling and
  // the TTL pixel clock.
  // To achieve this we try a number of PIO versions, each with a different
  // instruction per-pixel delay (IPP), and we calculate the PIO clock divider
  // needed, and how close that divider value is to the one we will actually get
  // given the 8-bit precision of the divider.
  // We choose the PIO IPP with the smallest divider error. This should give us
  // a pixel-perfect image.
  //

  // Find the best Pio instr delay by checking the error between the ideal
  // divider and what we get from the Pico's divider precision (256 fractional
  // positions).
  uint32_t BestIPP = 0;
  double BestErr = std::numeric_limits<double>::max();
  ClkDivider BestClkDiv;
  double PicoClk_Hz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS) * 1000;
  uint32_t PixelClk_Hz = getPxClkFor(TimingsTTL);
  auto IPPRange = getIPPRange(TimingsTTL.Mode);
  for (uint32_t IPP = IPPRange.first, E = IPPRange.second; IPP <= E; ++IPP) {
    double ClkDiv = (double)PicoClk_Hz / (PixelClk_Hz * IPP);
    ClkDivider ActualClkDivFloor(ClkDiv);
    ClkDivider ActualClkDivCeil(ClkDiv);
    ++ActualClkDivCeil;

    auto CheckDiv = [this, IPP, ClkDiv, &BestErr, &BestIPP,
                     &BestClkDiv](const ClkDivider &Div) {
      double Err = std::abs(Div.get() - ClkDiv);
      // DBG_PRINT(std::cout << "### IPP=" << IPP << " ClkDiv=" << ClkDiv
      //                     << " Div=" << Div << " Div.get()=" << Div.get()
      //                     << " Err=" << Err << "\n";)
      if (Err < BestErr) {
        BestErr = Err;
        BestIPP = IPP;
        BestClkDiv = Div;
      }
    };
    CheckDiv(ActualClkDivFloor);
    CheckDiv(ActualClkDivCeil);
  }
  DBG_PRINT(std::cout << "*** "
                      << " BestIPP=" << BestIPP << " BestErr=" << BestErr
                      << " BestClkDiv=" << BestClkDiv << "\n";)

  switch (TimingsTTL.Mode) {
  case TTL::CGA:
  case TTL::EGA:
    if (isHighRes(TimingsTTL)) {
      EGAIPP = BestIPP;
      EGAClkDiv = ClkDivider(BestClkDiv);
      DBG_PRINT(std::cout << "EGAClkDiv=" << EGAClkDiv << "\n";)
    } else {
      CGAIPP = BestIPP;
      CGAClkDiv = ClkDivider(BestClkDiv);
      DBG_PRINT(std::cout << "CGAClkDiv=" << CGAClkDiv << "\n";)
    }
    break;
  case TTL::MDA:
    MDAIPP = BestIPP;
    MDAClkDiv = ClkDivider(BestClkDiv);
    DBG_PRINT(std::cout << "MDAClkDiv=" << MDAClkDiv << "\n";)
    break;
  }
}

static void legalizeManualTTL(TTLDescrReduced &ManualTTL) {
  uint32_t VertMax = 0;
  uint32_t HorizMax = 0;
  switch (ManualTTL.Mode) {
  case TTL::CGA:
  case TTL::EGA:
    HorizMax = DisplayBuffer::BuffX + XB;
    VertMax = DisplayBuffer::BuffY + YB;
    break;
  case TTL::MDA:
    HorizMax = 2 * DisplayBuffer::BuffX + XB;
    VertMax = DisplayBuffer::BuffY + YB;
    break;
  }
  ManualTTL.V_Visible =
      std::clamp(ManualTTL.V_Visible, MANUAL_TTL_VERT_MIN, VertMax);
  ManualTTL.H_Visible =
      std::clamp(ManualTTL.H_Visible, MANUAL_TTL_HORIZ_MIN, HorizMax);
}

void TTLReader::saveToFlash(bool OnlyProfileBank, bool AllProfiles) {
  DBG_PRINT(std::cout << "Saving to flash...\n";)
  const int *FlashVec = Flash.getData();

  // Fill in the vector with the current values.
  FlashStorage::DataTy FlashValues;
  for (int Idx = 0, E = getNumFlashEntries(); Idx != E; ++Idx)
    FlashValues[Idx] = FlashVec[Idx];

  // Now override with the ones for the current preset (bank).
  FlashValues[ProfileBankIdx] = *ProfileBankOpt;

  if (!OnlyProfileBank) {
    uint32_t FromProfile = AllProfiles ? 0 : *ProfileBankOpt;
    uint32_t ToProfile = AllProfiles ? NumProfiles : FromProfile + 1;
    for (uint32_t Profile = FromProfile; Profile < ToProfile; ++Profile) {
      FlashValues[get(Profile::CGAPxClkIdx, Profile)] = CGAPxClk;
      FlashValues[get(Profile::CGASamplingOffsetIdx, Profile)] =
          CGASamplingOffset;
      FlashValues[get(Profile::EGAPxClkIdx, Profile)] = EGAPxClk;
      FlashValues[get(Profile::EGASamplingOffsetIdx, Profile)] =
          EGASamplingOffset;
      FlashValues[get(Profile::MDAPxClkIdx, Profile)] = MDAPxClk;
      FlashValues[get(Profile::MDASamplingOffsetIdx, Profile)] =
          MDASamplingOffset;
      FlashValues[get(Profile::CGABorderIdx, Profile)] =
          CGABorderOpt ? CGABorderOpt->getUint32() : InvalidBorder;
      FlashValues[get(Profile::EGABorderIdx, Profile)] =
          EGABorderOpt ? EGABorderOpt->getUint32() : InvalidBorder;
      FlashValues[get(Profile::MDABorderIdx, Profile)] =
          MDABorderOpt ? MDABorderOpt->getUint32() : InvalidBorder;
      FlashValues[get(Profile::ManualTTL_EnabledIdx, Profile)] = ManualTTLEnabled;
      FlashValues[get(Profile::ManualTTL_ModeIdx, Profile)] =
          ManualTTLEnabled ? getTTLIdx(ManualTTL.Mode) : 0;
      FlashValues[get(Profile::ManualTTL_H_VisibleIdx, Profile)] =
          ManualTTLEnabled ? ManualTTL.H_Visible : 0;
      FlashValues[get(Profile::ManualTTL_V_VisibleIdx, Profile)] =
          ManualTTLEnabled ? ManualTTL.V_Visible : 0;

      FlashValues[get(Profile::XBorderAUTOIdx, Profile)] =
          ManualTTLEnabled && XBorderAUTO ? XBorderAUTO : 0;
      FlashValues[get(Profile::ManualTTL_H_BackPorchIdx, Profile)] =
          ManualTTLEnabled ? ManualTTL.H_BackPorch : 0;

      FlashValues[get(Profile::YBorderAUTOIdx, Profile)] =
          ManualTTLEnabled && YBorderAUTO ? YBorderAUTO : 0;
      FlashValues[get(Profile::ManualTTL_V_BackPorchIdx, Profile)] =
          ManualTTLEnabled ? ManualTTL.V_BackPorch : 0;
    }
  }
  Flash.write(FlashValues);
  DBG_PRINT(std::cout << "DONE!\n";)
}

void TTLReader::toggleManualTTL() {
  if (ManualTTLEnabled) {
    ManualTTLEnabled = false;
  } else {
    ManualTTLEnabled = true;
    ManualTTL = TimingsTTL;     // Note this doesn't copy all values
  }
}

void TTLReader::printManualTTLMenu() {
  ManualTTLMenu.clearItems();
  // ON/OFF
  ManualTTLMenu.addMenuItem(
      ManualTTLMenu_Enabled_ItemIdx, true,
      /*Prefix=*/"",
      /*Item=*/ManualTTLEnabled ? "MANUAL-TTL" : "AUTO-TTL");
  // Mode (e.g., MDA/CGA/EGA
  ManualTTLMenu.addMenuItem(ManualTTLMenu_Mode_ItemIdx, ManualTTLEnabled,
                            /*Prefix=*/"", /*Item=*/modeToStr(ManualTTL.Mode));
  // Horizontal
  static Utils::StaticString<8> H_VisibleSS;
  H_VisibleSS = ManualTTL.H_Visible -
                /*XB is an implementation detail, hide it from user*/ XB;
  ManualTTLMenu.addMenuItem(ManualTTLMenu_Horiz_ItemIdx, ManualTTLEnabled,
                            /*Prefix=*/"", /*Item=*/
                            H_VisibleSS.get());
  // Vertical
  static Utils::StaticString<8> V_VisibleSS;
  V_VisibleSS = ManualTTL.V_Visible - YB;
  ManualTTLMenu.addMenuItem(ManualTTLMenu_Vert_ItemIdx, ManualTTLEnabled,
                            /*Prefix=*/"x",
                            /*Item=*/V_VisibleSS.get());

  // A switch for turning on/off XBorder setting
  ManualTTLMenu.addMenuItem(
      ManualTTLMenu_XBorderAUTO_ItemIdx, ManualTTLEnabled,
      /*Prefix=*/"X:", /*Item=*/XBorderAUTO ? "AUTO" : "MANUAL");

  static Utils::StaticString<8> H_BackPorchSS;
  H_BackPorchSS = ManualTTL.H_BackPorch;
  ManualTTLMenu.addMenuItem(ManualTTLMenu_XBorder_ItemIdx,
                            ManualTTLEnabled && !XBorderAUTO,
                            /*Prefix=*/"", /*Item=*/H_BackPorchSS.get());

  // A switch for turning on/off YBorder setting
  ManualTTLMenu.addMenuItem(
      ManualTTLMenu_YBorderAUTO_ItemIdx, ManualTTLEnabled,
      /*Prefix=*/"Y:", /*Item=*/YBorderAUTO ? "AUTO" : "MANUAL");

  static Utils::StaticString<8> V_BackPorchSS;
  V_BackPorchSS = ManualTTL.V_BackPorch;
  ManualTTLMenu.addMenuItem(ManualTTLMenu_YBorder_ItemIdx,
                            ManualTTLEnabled && !YBorderAUTO,
                            /*Prefix=*/"", /*Item=*/V_BackPorchSS.get());

  ManualTTLMenu.display(/*Selection=*/ManualTTLMenuIdx, MANUAL_TTL_DISPLAY_MS);
}

bool TTLReader::manualTTLMode() {
  // Turn ON ManualTTL on long-press of both buttons.
  if (UsrAction == UserAction::None) {
    DBG_PRINT(std::cout << "Manual TTL ON!\n";)
    UsrAction = UserAction::ManualTTL;
    // Start with the current mode.
    ManualTTL = TimingsTTL;
    // Point to "Mode"
    ManualTTLMenuIdx = ManualTTLEnabled ? 1 : 0;
    // Print the menu.
    printManualTTLMenu();
    // Set the timeout timer.
    auto Now = FrameEnd;
    ManualTTLExitTime = delayed_by_ms(Now, MANUAL_TTL_TIMEOUT_MS);
    // To avoid misclicks (because of the double long-press) wait until both
    // buttons are released to allow entering the menus.
    AllowEnterMenu = false;
    return true;
  }
  if (ManualTTLExitTime &&
      to_ms_since_boot(FrameEnd) > to_ms_since_boot(*ManualTTLExitTime)) {
    DBG_PRINT(std::cout << "Manual TTL before saveToFlash()\n";)
    saveToFlash();
    DBG_PRINT(std::cout << "MANUAL TTL SAVED TO FLASH!\n";)
    displayTxt("MANUAL TTL SAVED TO FLASH", MANUAL_TTL_DISPLAY_DONE_MS);
    // Turn off the timer.
    ManualTTLExitTime = std::nullopt;
    UsrAction = UserAction::None;
    return true;
  }
  // Because of the double long-press wait until the user has released both
  // buttons until we allow them to use the menus.
  if (!AllowEnterMenu &&
      (AutoAdjustBtn.get() == ButtonState::Release ||
       AutoAdjustBtn.get() == ButtonState::None) &&
      (PxClkBtn.get() == ButtonState::Release ||
       PxClkBtn.get() == ButtonState::None)) {
    AllowEnterMenu = true;
  }
  if (!AllowEnterMenu)
    return false;

  bool LongLeft = AutoAdjustBtn.get() == ButtonState::LongPress;
  bool LongRight = PxClkBtn.get() == ButtonState::LongPress;
  if (LongLeft || LongRight) {
    if (ManualTTLEnabled) {
      if (LongRight) {
        ManualTTLMenu.incrSelection(ManualTTLMenuIdx);
      } else if (LongLeft) {
        ManualTTLMenu.decrSelection(ManualTTLMenuIdx);
      }
    }
    printManualTTLMenu();
    return true;
  }
  // Manual TTL mode.
  bool Left = AutoAdjustBtn.get() == ButtonState::Release;
  bool Right = PxClkBtn.get() == ButtonState::Release;
  if (Left || Right) {
    if (Right) {
      switch (ManualTTLMenuIdx) {
      case ManualTTLMenu_Enabled_ItemIdx: {
        // ON/OFF
        toggleManualTTL();
        break;
      }
      case ManualTTLMenu_Mode_ItemIdx: {
        // Mode
        int NextIdx = (getTTLIdx(ManualTTL.Mode) + 1) % (MaxTTLIdx + 1);
        ManualTTL.Mode = getTTLAtIdx(NextIdx);
        break;
      }
      case ManualTTLMenu_Horiz_ItemIdx: {
        // Horizontal
        auto NextHoriz = ManualTTL.H_Visible + MANUAL_TTL_HORIZ_STEP;
        ManualTTL.H_Visible = NextHoriz;
        break;
      }
      case ManualTTLMenu_Vert_ItemIdx: {
        // Vertical
        auto NextVert = ManualTTL.V_Visible + MANUAL_TTL_VERT_STEP;
        ManualTTL.V_Visible = NextVert;
        break;
      }
      case ManualTTLMenu_XBorderAUTO_ItemIdx: {
        // XBorderAUTO switch
        XBorderAUTO = !XBorderAUTO;
        break;
      }
      case ManualTTLMenu_XBorder_ItemIdx: {
        // XBorder
        auto NextXBorder =
            std::min(MANUAL_TTL_MAX_XBORDER,
                     ManualTTL.H_BackPorch + MANUAL_TTL_XBORDER_STEP);
        ManualTTL.H_BackPorch = NextXBorder;
        break;
      }
      case ManualTTLMenu_YBorderAUTO_ItemIdx: {
        // YBorderAUTO switch
        YBorderAUTO = !YBorderAUTO;
        break;
      }
      case ManualTTLMenu_YBorder_ItemIdx: {
        // YBorder
        auto NextYBorder =
            std::min(MANUAL_TTL_MAX_YBORDER,
                     ManualTTL.V_BackPorch + MANUAL_TTL_YBORDER_STEP);
        ManualTTL.V_BackPorch = NextYBorder;
        break;
      }
      }
      legalizeManualTTL(ManualTTL);
      printManualTTLMenu();
      return true;
    }
    if (Left) {
      switch (ManualTTLMenuIdx) {
      case ManualTTLMenu_Enabled_ItemIdx: {
        // ON/OFF
        toggleManualTTL();
        break;
      }
      case ManualTTLMenu_Mode_ItemIdx: {
        // Mode
        int CurrIdx = getTTLIdx(ManualTTL.Mode);
        int PrevIdx = CurrIdx > 0 ? CurrIdx - 1 : MaxTTLIdx;
        ManualTTL.Mode = getTTLAtIdx(PrevIdx);
        break;
      }
      case ManualTTLMenu_Horiz_ItemIdx: {
        // Horizontal
        auto PrevHoriz = ManualTTL.H_Visible - MANUAL_TTL_HORIZ_STEP;
        ManualTTL.H_Visible = PrevHoriz;
        break;
      }
      case ManualTTLMenu_Vert_ItemIdx: {
        // Vertical
        auto PrevVert = ManualTTL.V_Visible - MANUAL_TTL_VERT_STEP;
        ManualTTL.V_Visible = PrevVert;
        break;
      }
      case ManualTTLMenu_XBorderAUTO_ItemIdx: {
        // XBorderAUTO switch
        XBorderAUTO = !XBorderAUTO;
        break;
      }
      case ManualTTLMenu_XBorder_ItemIdx: {
        // XBorder
        auto PrevXBorder =
            std::max(-MANUAL_TTL_MAX_XBORDER,
                     ManualTTL.H_BackPorch - MANUAL_TTL_XBORDER_STEP);
        ManualTTL.H_BackPorch = PrevXBorder;
        break;
      }
      case ManualTTLMenu_YBorderAUTO_ItemIdx: {
        // YBorderAUTO switch
        YBorderAUTO = !YBorderAUTO;
        break;
      }
      case ManualTTLMenu_YBorder_ItemIdx: {
        // YBorder
        auto PrevYBorder =
            std::max(-MANUAL_TTL_MAX_YBORDER,
                     ManualTTL.V_BackPorch - MANUAL_TTL_YBORDER_STEP);
        ManualTTL.V_BackPorch = PrevYBorder;
        break;
      }
      }
      legalizeManualTTL(ManualTTL);
      printManualTTLMenu();
      return true;
    }
  }
  return false;
}

void __not_in_flash_func(TTLReader::checkAndUpdateMode)() {
  // DBG_PRINT(std::cout << "Frame us=" << FrameUs << " Hz=" << VHz
  //                     << " Polarity=" << getPolarityStr(VSyncPolarity)
  //                     << " mode=" << modeToStr(Mode) << "\n";)
  std::optional<TTLDescr> NewModeOpt;
  if (ManualTTLEnabled) {
    TTLDescr Tmp;
    Tmp = ManualTTL;
    NewModeOpt = Tmp;
  } else {
    NewModeOpt = getModeForVPolarityAndHz(VSyncPolarity, VHz);
  }

  if (!NewModeOpt) {
    static uint32_t ThrottleMsgCnt;
    // Don't show the "unknown mode" message immediately wait until the
    // UnknownMsgMinCnt counter goes to 0.
    if (ThrottleMsgCnt++ % 64 == 0 && --UnknownMsgMinCnt == 0) {
      UnknownMsgMinCnt = UNKNOWN_MODE_SHOW_MSG_MIN_COUNT;
      DBG_PRINT(std::cout << "ManualTTLEnabled=" << ManualTTLEnabled << "\n";)
      DBG_PRINT(ManualTTL.dump(std::cout);)
      DBG_PRINT(std::cout << "Could not match Polarity="
                          << polarityToChar(VSyncPolarity) << " and Hz=" << VHz
                          << "\n";)
      // Display a helper debugging message that this is an unknown mode.
      // But limit the number of times the user will see the message.
      if (UsrAction == UserAction::None && UnknownMsgMaxCnt > 0) {
        --UnknownMsgMaxCnt;
        char Buff[40];
        snprintf(Buff, 40, "UNKNOWN MODE: VSYNC %dHz POLARITY:%c", (int)VHz,
                 polarityToChar(VSyncPolarity));
        displayTxt(Buff, UNKNOWN_MODE_MS);
      }
    }
    return;
  }
  // Reset the counter for the next time we get an unknown mode.
  UnknownMsgMaxCnt = UNKNOWN_MODE_SHOW_MSG_MAX_COUNT;
  UnknownMsgMinCnt = UNKNOWN_MODE_SHOW_MSG_MIN_COUNT;

  bool ChangeMode =
      *NewModeOpt != TimingsTTL; // NOTE: This ignore porches/retraces/Hz
  if (ChangeMode) {
    DBG_PRINT(std::cout << "\nChangeMode: " << *NewModeOpt << "\n";)
    DBG_PRINT(std::cout << "      From: " << TimingsTTL << "\n";)
    DBG_PRINT(std::cout << "OLD:\n";)
    Utils::StaticString<640> SS;
    TimingsTTL.dumpFull(SS, 0);
    DBG_PRINT(std::cout << SS.get() << "\n";)
    DBG_PRINT(std::cout << "NEW:\n";)
    SS.clear();
    NewModeOpt->dumpFull(SS, 0);
    DBG_PRINT(std::cout << SS.get() << "\n";);
    Buff.setMode(*NewModeOpt);
    TimingsTTL = *NewModeOpt;
    getDividerAutomatically();
    switchPio();
  }
}

void TTLReader::displayTTLInfo() {
  DBG_PRINT(std::cout << __FUNCTION__ << "\n";)
  TimingsTTL.PxClk = getPxClkFor(TimingsTTL);
  const auto &SamplingOffset = getSamplingOffsetFor(TimingsTTL);
  DBG_PRINT(std::cout << __FUNCTION__ << " after getSamplingOffsetFor()\n";)
  Utils::StaticString<640> SS;
  SS << "TTL INFO\n";
  SS << "--------\n";
  TimingsTTL.dumpFull(SS, SamplingOffset);
  Buff.displayPage(SS);
}

void TTLReader::showProfile() {
  DBG_PRINT(std::cerr << "PROFILE " << *ProfileBankOpt << "\n";)
  static constexpr const int BuffSz = 20;
  char Buff[BuffSz];
  snprintf(Buff, BuffSz, "PROFILE %lu", *ProfileBankOpt);
  displayTxt(Buff, PROFILE_DISPLAY_MS);
}

void TTLReader::changeProfile(bool Next) {
  DBG_PRINT(std::cerr << "changeProfile()\n";)
  if (Next) {
    *ProfileBankOpt += 1;
    if (ProfileBankOpt == NumProfiles)
      ProfileBankOpt = 0;
  } else {
    if (*ProfileBankOpt == 0)
      ProfileBankOpt = NumProfiles - 1;
    else
      *ProfileBankOpt -= 1;
  }
  showProfile();
  readConfigFromFlash();

  getDividerAutomatically();
  switchPio();
  checkAndUpdateMode();
}

void __not_in_flash_func(TTLReader::handleButtons)() {
  AutoAdjustBtn.tick();
  PxClkBtn.tick();

  auto BtnA = AutoAdjustBtn.get();
  auto BtnB = PxClkBtn.get();

  bool BothLongPress =
      (BtnA == ButtonState::Pressed && BtnB == ButtonState::LongPress) ||
      (BtnA == ButtonState::LongPress && BtnB == ButtonState::Pressed);

  if ((BothLongPress && UsrAction == UserAction::None) ||
      UsrAction == UserAction::ManualTTL) {
    // Long press to enter ManualTTL.
    if (manualTTLMode()) {
      // Reset the timer for exiting ManualTTL.
      ManualTTLExitTime = delayed_by_ms(FrameEnd, MANUAL_TTL_TIMEOUT_MS);

      checkAndUpdateMode();
    }
    return;
  }

  if (BtnB == ButtonState::LongPress &&
      UsrAction == UserAction::None) {
    if (NoSignal) {
      displayTxt("NO TTL SIGNAL", NO_TTL_SIGNAL_MS);
      return;
    }
    // If we have TTL signal then this prints TTL Info
    UsrAction = UserAction::TTLInfo;
    displayTTLInfo();
    return;
  }

  if (UsrAction == UserAction::TTLInfo &&
      (BtnA == ButtonState::Release || BtnA == ButtonState::LongPress ||
       BtnB == ButtonState::Release || BtnB == ButtonState::LongPress)) {
    // Quick exit from Info with a simple push.
    UsrAction = UserAction::None;
    Buff.clear();
    return;
  }

  if (UsrAction == UserAction::None) {
    if (BtnA == ButtonState::Release || BtnA == ButtonState::MedRelease) {
      if (NoSignal) {
        displayTxt("NO TTL SIGNAL", NO_TTL_SIGNAL_MS);
        return;
      }
      AutoAdjust.runAutoAdjust();
      return;
    }
    if (BtnA == ButtonState::LongPress) {
      if (NoSignal) {
        displayTxt("NO TTL SIGNAL", NO_TTL_SIGNAL_MS);
        return;
      }
      LastProfileBank = *ProfileBankOpt;
      showProfile();
      UsrAction = UserAction::ChangeProfile;
      ChangeProfileEndTime = delayed_by_ms(FrameEnd, PROFILE_DISPLAY_MS);
      return;
    }
  }

  if (UsrAction == UserAction::ChangeProfile) {
    if (BtnA == ButtonState::Release) {
      if (NoSignal) {
        displayTxt("NO TTL SIGNAL", NO_TTL_SIGNAL_MS);
        return;
      }
      changeProfile(/*Next=*/true);
      showProfile();
      ChangeProfileEndTime = delayed_by_ms(FrameEnd, PROFILE_DISPLAY_MS);
      return;
    }
    if (BtnB == ButtonState::Release) {
      if (NoSignal) {
        displayTxt("NO TTL SIGNAL", NO_TTL_SIGNAL_MS);
        return;
      }
      changeProfile(/*Next=*/false);
      showProfile();
      ChangeProfileEndTime = delayed_by_ms(FrameEnd, PROFILE_DISPLAY_MS);
      return;
    }
    if (to_ms_since_boot(FrameEnd) > to_ms_since_boot(*ChangeProfileEndTime)) {
      DBG_PRINT(std::cout << "ChangeProfile END!\n";)
      if (*ProfileBankOpt != LastProfileBank) {
        DBG_PRINT(std::cout << "Saving new preset " << *ProfileBankOpt
                            << " to flash\n";)
        saveToFlash(/*OnlyProfileBank=*/true);
      } else {
        DBG_PRINT(std::cout << "Profile didn't change\n";)
      }
      ChangeProfileEndTime = std::nullopt;
      UsrAction = UserAction::None;
      return;
    }
  }

  // Pixel clock adjustment
  if (UsrAction == UserAction::None ||
      UsrAction == UserAction::PxClkMode_Modify) {

    if (UsrAction == UserAction::None &&
        (BtnB == ButtonState::Release || BtnB == ButtonState::MedRelease)) {
      if (NoSignal) {
        displayTxt("NO TTL SIGNAL", NO_TTL_SIGNAL_MS);
        return;
      }
      // Enter pixel clk mode for the first time: display value but dont' change
      // it.
      displayPxClk();
      UsrAction = UserAction::PxClkMode_Modify;
      PxClkEndTime = delayed_by_ms(FrameEnd, PX_CLK_END_TIME_MS);
      ChangedPxClk = false;
      return;
    }
    // This is normal operation. We have already displayed the current PxClk,
    // now we can adjust it.
    if (BtnB == ButtonState::Release || BtnB == ButtonState::MedRelease ||
        BtnB == ButtonState::LongPress || BtnA == ButtonState::Release ||
        BtnA == ButtonState::MedRelease || BtnA == ButtonState::LongPress) {
      if (NoSignal) {
        displayTxt("NO TTL SIGNAL", NO_TTL_SIGNAL_MS);
        PxClkEndTime = std::nullopt;
        UsrAction = UserAction::None;
        return;
      }
      ChangedPxClk = true;
      UsrAction = UserAction::PxClkMode_Modify;
      bool IncreasePxClk = BtnB == ButtonState::Release ||
                           BtnB == ButtonState::MedRelease ||
                           BtnB == ButtonState::LongPress;
      PxClkEndTime = delayed_by_ms(FrameEnd, PX_CLK_END_TIME_MS);
      bool SmallStep =
          BtnA == ButtonState::LongPress || BtnB == ButtonState::LongPress;
      bool OffsetStep =
          BtnA == ButtonState::MedRelease || BtnB == ButtonState::MedRelease;
      changePxClk(/*Increase=*/IncreasePxClk, SmallStep, OffsetStep);
      getDividerAutomatically();
      switchPio();
      checkAndUpdateMode();
      return;
    }
    if (PxClkEndTime &&
        to_ms_since_boot(FrameEnd) > to_ms_since_boot(*PxClkEndTime)) {
      PxClkEndTime = std::nullopt;
      if (ChangedPxClk) {
        DBG_PRINT(std::cout << "Before saveToFlash() in ChangePxClk\n";)
        saveToFlash();
        displayTxt("PxCLK SAVED TO FLASH", PX_CLK_EXIT_DISPLAY_MS);
      } else {
        displayTxt("PxCLK UNCHANGED", PX_CLK_EXIT_DISPLAY_MS);
      }
      UsrAction = UserAction::None;
      return;
    }
  }
}

void TTLReader::displayTxt(const char *Txt, int Time) {
  DBG_PRINT(std::cout << "TTLReader::" << __FUNCTION__ << " Txt=" << Txt
                      << " Time=" << Time << "\n";)
  Buff.displayTxt(Txt, 0, /*Center=*/true);
  DisplayTxtEndTime = delayed_by_ms(get_absolute_time(), Time);
}

void TTLReader::displayTxtTick() {
  if (DisplayTxtEndTime) {
    if (to_ms_since_boot(get_absolute_time()) <
        to_ms_since_boot(*DisplayTxtEndTime)) {
      Buff.copyTxtBufferToScreen();
    } else {
      DBG_PRINT(std::cout << "DisplayTxtEnd\n";)
      DisplayTxtEndTime = std::nullopt;
      if (NoSignal)
        Buff.noSignal();
    }
  }
}

/// \Returns true if \p Descr is high resolution. This is to tell apart EGA
/// 640x350 from 640x200.
bool TTLReader::isHighRes(const TTLDescr &Descr) {
  // Ideally the limit should be 240, but:
  // - Some non-standard inputs are 260 lines instead of 240
  // - and we currently don't support 800x600 for CGA/EGA (not fast enough)
  // so we go over the limit and still use 640x480 with line doubling for these
  // modes in order to preserve the aspect ratio.
  return Descr.V_Visible - YB > 260;
}

template <TTL M, bool DiscardLineData>
bool __not_in_flash_func(TTLReader::readLinePerMode)(uint32_t Line) {
  bool InVSync = false;
  if constexpr (M == TTL::CGA) {
    InVSync = readLineCGA<DiscardLineData>(Line);
    AutoAdjust.collect(TTLBorderPio, TTLBorderSM, CGABorderCounter, Line,
                       TimingsTTL.Mode);
  } else if constexpr (M == TTL::EGA) {
    InVSync = readLineCGA<DiscardLineData>(Line);
    AutoAdjust.collect(TTLBorderPio, TTLBorderSM, EGABorderCounter, Line,
                       TimingsTTL.Mode);
  } else if constexpr (M == TTL::MDA) {
    InVSync = readLineMDA<DiscardLineData>(Line);
    AutoAdjust.collect(TTLBorderPio, TTLBorderSM, MDABorderCounter, Line,
                       TimingsTTL.Mode);
  }
  else {
    DBG_PRINT(std::cout << "Bad mode: " << modeToStr(TimingsTTL.Mode) << "\n";)
    Utils::sleep_ms(1000);
  }
  return InVSync;
}

void TTLReader::calculateVHSyncs(absolute_time_t LastFrameEnd,
                                 uint32_t VisibleLines) {
  uint32_t FrameUs = absolute_time_diff_us(LastFrameEnd, FrameEnd);
  VHz = (float)1000000 / FrameUs;

  uint32_t VisibleUs = absolute_time_diff_us(FrameBegin, FrameEnd);
  HHz = (float)1000000 * VisibleLines / VisibleUs;

  FrameBegin = FrameEnd;
}

void TTLReader::setBorders() {
  switch (TimingsTTL.Mode) {
  case TTL::CGA:
  case TTL::EGA:
    if (isHighRes(TimingsTTL)) {
      if (EGABorderOpt)
        AutoAdjust.setBorder(*EGABorderOpt);
    } else {
      if (CGABorderOpt)
        AutoAdjust.setBorder(*CGABorderOpt);
    }
    break;
  case TTL::MDA:
    if (MDABorderOpt)
      AutoAdjust.setBorder(*MDABorderOpt);
    break;
  }
}

template <TTL M>
void __not_in_flash_func(TTLReader::readFrame)(uint32_t &Line) {
  if (DisplayTxtEndTime) {
    // If we are displaying on-screen text use this code block.
    bool InVSync = false;
    do {
      uint32_t VisibleLine = Line - YBorder;
      bool DiscardLine = DisplayTxtEndTime &&
                         VisibleLine >= Buff.getTxtLineYTop() &&
                         VisibleLine <= Buff.getTxtLineYBot();
      if (DiscardLine) {
        InVSync = readLinePerMode<M, /*DiscardLine=*/true>(Line);
      } else {
        InVSync = readLinePerMode<M, /*DiscardLine=*/false>(Line);
      }
      ++Line;
    } while (!InVSync);
  } else {
    // Not displaying any text, optimized block.
    bool InVSync = false;
    do {
      InVSync = readLinePerMode<M, /*DiscardLine=*/false>(Line);
      ++Line;
    } while (!InVSync);
    // Fill the bottom of the frame buffer with black pixels to remove
    // out-of-border artifacts that may show up when closing programs.
    Buff.fillBottomWithBlackAfter(Line);
  }
}

void TTLReader::runForEver() {
  Pi.ledON();
  while (true) {
    ++FrameCnt;
    bool InInfoPage = UsrAction == UserAction::TTLInfo;
    bool DisableInput = NoSignal || InInfoPage;
    // Wait here if we are in VSync retrace.
    bool RetraceVSync = TimingsTTL.V_SyncPolarity == Pos;
    while (!DisableInput && gpio_get(TTL_VSYNC_GPIO) == RetraceVSync)
      ;
    FrameBegin = get_absolute_time();
    // A fresh frame, start with Line 0
    uint32_t Line = 0;
    if (ManualTTLEnabled)
      TimingsTTL = ManualTTL;

    if (!DisableInput) {
      switch (TimingsTTL.Mode) {
      case TTL::EGA:
        readFrame<TTL::EGA>(Line);
        break;
      case TTL::CGA:
        readFrame<TTL::CGA>(Line);
        break;
      case TTL::MDA:
        readFrame<TTL::MDA>(Line);
        break;
      }
    }

    if (DisableInput) {
      if (NoSignal != LastNoSignal)
        Buff.noSignal();
      // Simulate a frame delay, since we are not actually receiving TTL data.
      // This helps with button timings.
      Utils::sleep_ms(20);
    }
    LastNoSignal = NoSignal;

    bool BordersAdjusted = AutoAdjust.frameTick(TimingsTTL);
    if (BordersAdjusted) {
      DBG_PRINT(std::cout << "Borders adjusted!\n";)
      saveToFlash();
    }

    auto LastFrameEnd = FrameEnd;
    FrameEnd = get_absolute_time();

    auto Mod = FrameCnt % TTL_MOD;
    if (!DisableInput && Mod == 1)
      calculateVHSyncs(LastFrameEnd, Line);

    if (!DisableInput) {
      if (Mod == 2)
        updateSyncPolarityVariables();
      else if (Mod == 3)
        checkAndUpdateMode();
    }
    // Try set the border from flash values.
    if (!DisableInput && Mod == 0)
      setBorders();

    handleButtons();
    if (Mod == 0)
      displayTxtTick();
  }
}
