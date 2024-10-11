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

extern class DisplayBuffer Buff;

static constexpr const uint32_t EGABorderCounter = 700;
static constexpr const uint32_t CGABorderCounter = 700;
static constexpr const uint32_t MDABorderCounter = 800;

static inline void EGA640x350PioConfig(PIO Pio, uint SM, uint Offset,
                                       uint RGB_GPIO, uint32_t HSYNC_GPIO,
                                       uint16_t ClkDivInt, uint8_t ClkDivFrac,
                                       uint32_t InstrDelay) {
  auto GetInstrDelay = [Offset](uint32_t InstrDelay) {
    switch (InstrDelay) {
      // case 7:
      // return EGA640x350_07_program_get_default_config(Offset);
#include "EGASwitchCase_config_cpp"
    }
    std::cerr << "Bad InstrDelay EGA: GetInstrDelay(" << InstrDelay << ")\n";
    exit(1);
  };
  pio_sm_config Conf = GetInstrDelay(InstrDelay);

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
                                       uint32_t InstrDelay) {
  auto GetInstrDelay = [Offset](uint32_t InstrDelay) {
    switch (InstrDelay) {
      // case 9:
      // return CGA640x200_09_program_get_default_config(Offset);
#include "CGASwitchCase_config_cpp"
    }
    std::cerr << "Bad InstrDelay CGA: GetInstrDelay(" << InstrDelay << ")\n";
    exit(1);
  };
  pio_sm_config Conf = GetInstrDelay(InstrDelay);
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
                                       uint32_t InstrDelay) {
  auto GetInstrDelay = [Offset](uint32_t InstrDelay) {
    switch (InstrDelay) {
#include "MDASwitchCase_config_cpp"
    }
    std::cerr << "GetInstrDelay MDA Bad InstrDelay " << InstrDelay << "\n";
    exit(1);
  };
  pio_sm_config Conf = GetInstrDelay(InstrDelay);
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

void AutoAdjustBorder::applyBorders(Resolution Mode) {
  // Clear screen if borders have changed to avoid artifacts.
  if (XBorder != TmpXBorder || YBorder != TmpYBorder)
    Buff.clear();

  // Update TTLReader's values.
  XBorder = TmpXBorder;
  YBorder = TmpYBorder;

  switch (Mode) {
  case CGA_640x200_60Hz:
  case EGA_640x200_60Hz:
    CGABorderOpt = BorderXY(XBorder, YBorder);
    break;
  case EGA_640x350_60Hz:
    EGABorderOpt = BorderXY(XBorder, YBorder);
    break;
  case MDA_720x350_50Hz:
    MDABorderOpt = BorderXY(XBorder, YBorder);
    break;
  }
}

void AutoAdjustBorder::setBorder(const BorderXY &XY) {
  XBorder = XY.X;
  YBorder = XY.Y;
}

void AutoAdjustBorder::forceStart(bool AlwaysON) {
  if (Enabled == State::Off)
    Enabled = AlwaysON ? State::AlwaysON : State::SingleON;
  FrameCnt = 0u;
  StartFrame = FrameCnt + AUTO_ADJUST_START_AFTER;
  StopFrame = FrameCnt + AUTO_ADJUST_DURATION;
}

void AutoAdjustBorder::tryPushButtonStart() {
  if (Btn.get() == ButtonState::Release) {
    DBG_PRINT(std::cout << "Auto Adjusting...\n";)
    Buff.displayTxt("AUTO ADJUST", 0, 0, DISPLAY_TXT_ZOOM, /*Center=*/true);
    sleep_ms(AUTO_ADJUST_DISPLAY_MS);
    Buff.clear();
    forceStart(/*AlwaysON=*/false);
  } else if (Btn.get() == ButtonState::LongPress) {
    if (Enabled == State::Off) {
      DBG_PRINT(std::cout << "Auto Adjust ALWAYS ON\n";)
      Buff.displayTxt("AUTO ADJUST\nALWAYS ON", 0, 0, DISPLAY_TXT_ZOOM,
                      /*Center=*/true);
      sleep_ms(AUTO_ADJUST_ALWAYS_ON_DISPLAY_MS);
      Buff.clear();
      forceStart(/*AlwaysON=*/true);
    } else if (Enabled == State::AlwaysON) {
      DBG_PRINT(std::cout << "Auto Adjust ALWAYS ON\n";)
      Buff.displayTxt("AUTO ADJUST\nMANUAL", 0, 0, DISPLAY_TXT_ZOOM,
                      /*Center=*/true);
      sleep_ms(AUTO_ADJUST_ALWAYS_ON_DISPLAY_MS);
      Enabled = State::Off;
    }
  }
}

bool AutoAdjustBorder::frameTick(Resolution Mode) {
  if (Enabled == State::Off)
    return false;
  if (++ThrottleCnt % AUTO_ADJUST_THROTTLE_CNT == 0)
    return false;
  ++FrameCnt;
  if (FrameCnt == StartFrame) {
    resetBorders();
  } else if (FrameCnt == StopFrame) {
    applyBorders(Mode);
    DBG_PRINT(std::cout << "Auto Adust: XBorder=" << XBorder
                        << " YBorder=" << YBorder << "\n";)
    switch (Enabled) {
    case State::SingleON:
      Enabled = State::Off;
      break;
    case State::AlwaysON:
      forceStart(/*AlwaysON=*/true);
      break;
    }
    return true;
  }
  return false;
}

void AutoAdjustBorder::collect(PIO Pio, uint32_t SM, uint32_t ModeBorderCounter,
                               uint32_t Line, Resolution M) {
  // Get the border value from the dedicated PIO.
  uint32_t Raw = pio_sm_get_blocking(Pio, SM);
  if (Raw == 0xffffffff)
    Raw = 0;

  auto GetPixelsPerEntry = [](Resolution M) {
    switch (M) {
    case EGA_640x350_60Hz:
    case CGA_640x200_60Hz:
      return 4;
    case MDA_720x350_50Hz:
      return 8;
    default:
      std::cerr << "GetPixelsPerEntry() BAD Mode " << modeToStr(M) << "\n";
      exit(1);
    }
  };

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

uint32_t &TTLReader::getPxClkFor(Resolution M) {
  switch (M) {
  case EGA_640x350_60Hz:
    return EGAPxClk;
  case EGA_640x200_60Hz:
  case CGA_640x200_60Hz:
    return CGAPxClk;
  case MDA_720x350_50Hz:
    return MDAPxClk;
  default:
    std::cerr << __FUNCTION__ << " BAD Mode " << (int)M << "\n";
    exit(1);
  }
}

void TTLReader::changePxClk(bool Increase, bool SmallStep) {
  /// \Returns the ClkDiv for the current mode.
  uint32_t &PixelClock = getPxClkFor(Buff.getMode());
  DBG_PRINT(std::cout << "\n-----------\n";)
  DBG_PRINT(std::cout << "Pixel Clock Before=" << PixelClock;)
  if (Increase) {
    DBG_PRINT(std::cout << " ++ ";)
    PixelClock += SmallStep ? 1000 : 10000;
  } else {
    DBG_PRINT(std::cout << " -- ";)
    PixelClock -= SmallStep ? 1000 : 10000;
  }
  DBG_PRINT(std::cout << "After=" << PixelClock << "\n";)
  DBG_PRINT(std::cout << "-----------\n\n";)
  static constexpr const int BuffSz = 18;
  char Txt[BuffSz];
  snprintf(Txt, BuffSz, "PxCLK:%2.3fMHz", (float)PixelClock / 1000000);
  Buff.displayTxt(Txt, 0, 0, DISPLAY_TXT_ZOOM, /*Center=*/true);
  sleep_ms(PX_CLK_TXT_DISPLAY_MS);
}

void TTLReader::setTimingNOPs() {
  uint32_t TimingNOPs = Buff.getPixelClock_PIO();
  DBG_PRINT(std::cout << "Set timing NOPs to " << TimingNOPs << "\n";)
  pio_sm_put_blocking(TTLPio, TTLSM, TimingNOPs);
  DBG_PRINT(std::cout << "Done\n";)
}

TTLReader::TTLReader(PioProgramLoader &PioLoader, Pico &Pi, FlashStorage &Flash)
    : PioLoader(PioLoader), AutoAdjustBtn(AUTO_ADJUST_GPIO, Pi, "AutoAdjust"),
      PxClkBtn(PX_CLK_BTN_GPIO, Pi, "PxClk"),
      AutoAdjust(XBorder, YBorder, CGABorderOpt, EGABorderOpt, MDABorderOpt,
                 AutoAdjustBtn, Flash),
      Flash(Flash) {
  // Load PIO clock dividers from Flash
  if (Flash.valid()) {
    DBG_PRINT(std::cout << "\nReading from flash...\n";)
    CGAPxClk = (uint32_t)Flash.read(CGAPxClkIdx);
    EGAPxClk = (uint32_t)Flash.read(EGAPxClkIdx);
    MDAPxClk = (uint32_t)Flash.read(MDAPxClkIdx);

    auto ReadBorderSafe = [this, &Flash](int Idx) -> std::optional<BorderXY> {
      uint32_t XY = (uint32_t)Flash.read(Idx);
      // If 0xFFFFFFFF then it's not valid
      if (XY == InvalidBorder)
        return std::nullopt;
      return BorderXY(XY);
    };

    CGABorderOpt = ReadBorderSafe(CGABorderIdx);
    EGABorderOpt = ReadBorderSafe(EGABorderIdx);
    MDABorderOpt = ReadBorderSafe(MDABorderIdx);

    DBG_PRINT(std::cout << "CGAPxClk=" << CGAPxClk << "\n";)
    DBG_PRINT(std::cout << "EGAPxClk=" << EGAPxClk << "\n";)
    DBG_PRINT(std::cout << "MDAPxClk=" << MDAPxClk << "\n";)
  } else {
    DBG_PRINT(std::cout << "Flash not valid!\n";);
  }

  TTLPio = pio0;
  TTLSM = pio_claim_unused_sm(TTLPio, true);

  TTLBorderPio = pio0;
  TTLBorderSM = pio_claim_unused_sm(TTLBorderPio, true);

  // Start the VSyncPolarity PIO.
  VSyncPolarityPio = pio0;
  VSyncPolaritySM = pio_claim_unused_sm(VSyncPolarityPio, true);
  VSyncPolarityOffset = PioLoader.loadPIOProgram(
      VSyncPolarityPio, VSyncPolaritySM, &VSyncPolarity_program,
      [this](PIO Pio, uint SM, uint Offset) {
        VSyncPolarityPioConfig(Pio, SM, Offset, TTL_VSYNC_GPIO);
      });
  pio_sm_put_blocking(VSyncPolarityPio, VSyncPolaritySM, VSyncPolarityCounter);

  getDividerAutomatically();
  switchPio();
}

template bool TTLReader::readLineCGA<CGA_640x200_60Hz>(uint32_t);
template bool TTLReader::readLineCGA<EGA_640x350_60Hz>(uint32_t);

template <uint32_t M>
bool TTLReader::readLineCGA(uint32_t Line) {
  // Now fill in the line until HSync is high.
  uint32_t X = 0;

  uint32_t XBorderAdj =
      (XBorder + /*FIFO sz=*/8 * /*Pixels per FIFO Entry=*/4) &
      0xfffffffc;                                    // Must be 4-byte aligned!
  // Wait here if we are still in HSync retrace
  while (gpio_get(TTL_HSYNC_GPIO) != 0)
    ;
  if (Line < YBorder) {
    while (gpio_get(TTL_HSYNC_GPIO) == 0)
      ;
  } else {
    uint32_t XMax = Timing[M][H_Visible] + XBorderAdj;
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
      if (X >= XBorderAdj) {
        Buff.setCGA32(Line - YBorder, X - XBorderAdj, VHRGB & RGBMask_4);
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
      gpio_get(TTL_VSYNC_GPIO) == (Timing[M][V_SyncPolarity] == Pos);
  DBG_PRINT(MaxX = std::max(MaxX, X);)
  return InRetrace;
}

bool TTLReader::readLineMDA(uint32_t Line) {
  static constexpr const auto M = MDA_720x350_50Hz;
  // Now fill in the line until HSync is high.
  uint32_t PixelX = 0;
  uint32_t BuffX = 0;
  uint32_t XBorderAdj = XBorder + /*FIFO sz=*/8 * /*Pixels per FIFO Entry=*/8;
  // Divide by 2 because we are packing 2 pixels per byte.
  uint32_t BuffXBorder = XBorderAdj / 2 & 0xfffffffc; // Must be 4-byte aligned!

  // Wait here if we are still in HSync retrace
  while (gpio_get(TTL_HSYNC_GPIO) != 0)
    ;
  if (Line < YBorder) {
    while (gpio_get(TTL_HSYNC_GPIO) == 0)
      ;
  } else {
    uint32_t XMax = Timing[M][H_Visible];
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
      if (BuffX >= BuffXBorder)
        Buff.setMDA32(Line - YBorder, BuffX - BuffXBorder, MDA8);
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
      gpio_get(TTL_VSYNC_GPIO) == (Timing[M][V_SyncPolarity] == Pos);
  DBG_PRINT(MaxX = std::max(MaxX, PixelX);)
  return InRetrace;
}

static auto getEGAProgram(uint32_t IPP) {
  switch(IPP) {
    #include "EGASwitchCase_program_cpp"
  }
  std::cerr << "Bad IPP getEGAProgram(" << IPP << ")\n";
  exit(1);
}
static auto getCGAProgram(uint32_t IPP) {
  switch(IPP) {
    #include "CGASwitchCase_program_cpp"
  }
  std::cerr << "Bad IPP getCGAProgram(" << IPP << ")\n";
  exit(1);
}

static auto getMDAProgram(uint32_t IPP) {
  switch(IPP) {
    #include "MDASwitchCase_program_cpp"
  }
  std::cerr << "Bad IPP getMDAProgram(" << IPP << ")\n";
  exit(1);
}

static std::pair<uint32_t, uint32_t> getIPPRange(Resolution M) {
  switch (M) {
  case EGA_640x350_60Hz:
    return {4, 16};
  case CGA_640x200_60Hz:
  case EGA_640x200_60Hz:
    return {4, 16};
  case MDA_720x350_50Hz:
    return {5, 16};
  }
  std::cerr << "Bad Mode in getIPPRange(" << modeToStr(M) << ")\n";
  exit(1);
}

bool TTLReader::haveBorderFromFlash(Resolution M) const {
  switch (M) {
  case CGA_640x200_60Hz:
  case EGA_640x200_60Hz:
    return (bool)CGABorderOpt;
  case EGA_640x350_60Hz:
    return (bool)EGABorderOpt;
  case MDA_720x350_50Hz:
    return (bool)MDABorderOpt;
  }
  return false;
}

void TTLReader::switchPio() {
  PioLoader.unloadAllPio(TTLPio, {TTLSM, TTLBorderSM});
  DBG_PRINT(std::cout << "\nSwitching PIO to " << modeToStr(Buff.getMode())
                      << "\n";)

  auto GetBorderIPP = [](Resolution M) {
    switch (M) {
    case EGA_640x350_60Hz:
      return 10;
    case EGA_640x200_60Hz:
    case CGA_640x200_60Hz:
      return 10;
    case MDA_720x350_50Hz:
      return 10;
    default:
      std::cerr << " GetBorderIPP BAD Mode " << modeToStr(M) << "\n";
      exit(1);
    }
  };

  auto M = Buff.getMode();
  double PicoClk_Hz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS) * 1000;
  float BorderClkDiv = PicoClk_Hz / (getPxClkFor(M) * GetBorderIPP(M));
  DBG_PRINT(std::cout << "\n\nBORDER CLKDIV=" << BorderClkDiv << "\n";)

  switch (M) {
  case MDA_720x350_50Hz: {
    TTLOffset = PioLoader.loadPIOProgram(
        TTLPio, TTLSM, getMDAProgram(MDAIPP),
        [this](PIO Pio, uint SM, uint Offset) {
          MDA720x350PioConfig(Pio, SM, Offset, MDA_VI_GPIO, TTL_HSYNC_GPIO,
                              MDAClkDiv.getInt(), MDAClkDiv.getFrac(), MDAIPP);
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
  case CGA_640x200_60Hz:
  case EGA_640x200_60Hz: {
    TTLOffset = PioLoader.loadPIOProgram(
        TTLPio, TTLSM, getCGAProgram(CGAIPP),
        [this](PIO Pio, uint SM, uint Offset) {
          CGA640x200PioConfig(Pio, SM, Offset, CGA_ACTUAL_RGB_GPIO,
                              TTL_HSYNC_GPIO, CGAClkDiv.getInt(),
                              CGAClkDiv.getFrac(), CGAIPP);
        });

    TTLBorderOffset = PioLoader.loadPIOProgram(
        TTLBorderPio, TTLBorderSM, &CGA640x200Border_program,
        [BorderClkDiv](PIO Pio, uint SM, uint Offset) {
          CGA640x200BorderPioConfig(Pio, SM, Offset, CGA_ACTUAL_RGB_GPIO,
                                    BorderClkDiv);
        });
    pio_sm_put_blocking(TTLBorderPio, TTLBorderSM, /*Counter=*/CGABorderCounter);
    break;
  }
  case EGA_640x350_60Hz: {
    TTLOffset = PioLoader.loadPIOProgram(
        TTLPio, TTLSM, getEGAProgram(EGAIPP),
        [this](PIO Pio, uint SM, uint Offset) {
          EGA640x350PioConfig(Pio, SM, Offset, EGA_RGB_GPIO, TTL_HSYNC_GPIO,
                              EGAClkDiv.getInt(), EGAClkDiv.getFrac(), EGAIPP);
        });

    TTLBorderOffset = PioLoader.loadPIOProgram(
        TTLBorderPio, TTLBorderSM, &EGA640x350Border_program,
        [BorderClkDiv](PIO Pio, uint SM, uint Offset) {
          EGA640x350BorderPioConfig(Pio, SM, Offset, EGA_RGB_GPIO,
                                    BorderClkDiv);
        });
    pio_sm_put_blocking(TTLBorderPio, TTLBorderSM,
                        /*Counter=*/EGABorderCounter);
    break;
  }
  }

  if (!haveBorderFromFlash(M))
    AutoAdjust.forceStart(/*AlwaysON=*/false);
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
  auto M = Buff.getMode();
  uint32_t LinePixels = Timing[M][H_FrontPorch] + Timing[M][H_Visible] +
                        Timing[M][H_BackPorch] + Timing[M][H_Sync];

  // Find the best Pio instr delay by checking the error between the ideal
  // divider and what we get from the Pico's divider precision (256 fractional
  // positions).
  uint32_t BestIPP;
  double BestErr = std::numeric_limits<double>::max();
  ClkDivider BestClkDiv;
  double PicoClk_Hz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS) * 1000;
  uint32_t PixelClk_Hz = getPxClkFor(M);
  auto IPPRange = getIPPRange(M);
  for (uint32_t IPP = IPPRange.first, E = IPPRange.second; IPP <= E; ++IPP) {
    double ClkDiv = (double)PicoClk_Hz / (PixelClk_Hz * IPP);
    ClkDivider ActualClkDivFloor(ClkDiv);
    ClkDivider ActualClkDivCeil(ClkDiv);
    ++ActualClkDivCeil;

    auto CheckDiv = [this, IPP, ClkDiv, &BestErr, &BestIPP,
                     &BestClkDiv](const ClkDivider &Div) {
      double Err = std::abs(Div.get() - ClkDiv);
      DBG_PRINT(std::cout << "### IPP=" << IPP << " ClkDiv=" << ClkDiv
                          << " Div=" << Div << " Div.get()=" << Div.get()
                          << " Err=" << Err << "\n";)
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

  switch (M) {
  case EGA_640x350_60Hz:
    EGAIPP = BestIPP;
    EGAClkDiv = ClkDivider(BestClkDiv);
    DBG_PRINT(std::cout << "EGAClkDiv=" << EGAClkDiv << "\n";)
    break;
  case EGA_640x200_60Hz:
  case CGA_640x200_60Hz:
    CGAIPP = BestIPP;
    CGAClkDiv = ClkDivider(BestClkDiv);
    DBG_PRINT(std::cout << "CGAClkDiv=" << CGAClkDiv << "\n";)
    break;
  case MDA_720x350_50Hz:
    MDAIPP = BestIPP;
    MDAClkDiv = ClkDivider(BestClkDiv);
    DBG_PRINT(std::cout << "MDAClkDiv=" << MDAClkDiv << "\n";)
    break;
  }
}

bool TTLReader::getVSyncPolarity() const {
  if (pio_sm_is_rx_fifo_empty(VSyncPolarityPio, VSyncPolaritySM)) {
    DBG_PRINT(std::cerr << "ERROR: getVSyncPolarity() empty!\n";)
  }
  uint32_t Raw = pio_sm_get_blocking(VSyncPolarityPio, VSyncPolaritySM);
  uint32_t VSyncLowCnt = VSyncPolarityCounter - Raw;
  static constexpr const uint32_t LoopInstrs = 2u; // 2 instrs in count loop
  static constexpr const uint32_t InstrNs = 4u;    // actually 3.7ns
  uint32_t LowNs = LoopInstrs * InstrNs * VSyncLowCnt;
  auto M = Buff.getMode();
  uint32_t VertNs = (float)1000000000 / Timing[M][V_Hz];
  return LowNs > VertNs / 2 ? Pos : Neg;
}

void TTLReader::saveToFlash() {
  DBG_PRINT(std::cout << "Saving to flash...\n";)
  std::vector<int> FlashValues;
  FlashValues.resize((int)MaxFlashIdx);
  FlashValues[CGAPxClkIdx] = CGAPxClk;
  FlashValues[EGAPxClkIdx] = EGAPxClk;
  FlashValues[MDAPxClkIdx] = MDAPxClk;
  FlashValues[CGABorderIdx] =
      CGABorderOpt ? CGABorderOpt->getUint32() : InvalidBorder;
  FlashValues[EGABorderIdx] =
      EGABorderOpt ? EGABorderOpt->getUint32() : InvalidBorder;
  FlashValues[MDABorderIdx] =
      MDABorderOpt ? MDABorderOpt->getUint32() : InvalidBorder;
  Flash.write(FlashValues);
  DBG_PRINT(std::cout << "DONE!\n";)
}

bool TTLReader::tryChangePixelClock() {
  auto AutoAdjustBtnState = AutoAdjustBtn.get();
  auto PxClkBtnState = PxClkBtn.get();

  bool PxClkBtnPressed = PxClkBtnState == ButtonState::Release ||
                         PxClkBtnState == ButtonState::MedRelease;
  if (PxClkBtnPressed) {
    if (AdjPxClkMode != PxClkMode::Modify) {
      // If we just entered Pixel Clock adjust mode, show message on screen.
      static constexpr const int BuffSz = 34;
      char PxClkTxt[BuffSz];
      uint32_t &PixelClock = getPxClkFor(Buff.getMode());
      snprintf(PxClkTxt, BuffSz, "%s\nPxCLK:%2.3fMHz",
               modeToStr(Buff.getMode()), (float)PixelClock / 1000000);
      Buff.displayTxt(PxClkTxt, 0, 0, DISPLAY_TXT_ZOOM, /*Center=*/true);
      sleep_ms(PX_CLK_INITIAL_TXT_DISPLAY_MS);
      AdjPxClkMode = PxClkMode::Modify;
      AdjPxClkCnt = ADJUST_PX_CLK_CNT;
      return false;
    }
    AdjPxClkMode = PxClkMode::Modify; // Not in reset
  }
  if (AdjPxClkMode == PxClkMode::None)
    return false;

  if (PxClkBtnState == ButtonState::LongPress) {
    // Save to flash and return.
    saveToFlash();
    Buff.displayTxt("PxCLK SAVED TO FLASH", 0, 0, DISPLAY_TXT_ZOOM,
                    /*Center=*/true);
    sleep_ms(PX_CLK_TXT_DISPLAY_MS + 1000);
    AdjPxClkMode = PxClkMode::None;
    return false;
  }
  if (AutoAdjustBtnState == ButtonState::LongPress) {
    if (AdjPxClkMode == PxClkMode::Modify) {
      Buff.displayTxt("RESET PxCLK TO DEFAULTS?", 0, 0, DISPLAY_TXT_ZOOM,
                      /*Center=*/true);
      sleep_ms(PX_CLK_RESET_DISPLAY_MS);
      AdjPxClkMode = PxClkMode::Reset;
      return false;
    } else if (AdjPxClkMode == PxClkMode::Reset) {
      CGAPxClk = Timing[CGA_640x200_60Hz][PxClk];
      EGAPxClk = Timing[EGA_640x350_60Hz][PxClk];
      MDAPxClk = Timing[MDA_720x350_50Hz][PxClk];
      saveToFlash();
      getDividerAutomatically();
      switchPio();
      Buff.displayTxt("RESET OK", 0, 0, DISPLAY_TXT_ZOOM, /*Center=*/true);
      sleep_ms(PX_CLK_RESET_DISPLAY_MS);
      AdjPxClkMode = PxClkMode::None;
      return false;
    }
  }
  bool AutoAdjBtnPressed = AutoAdjustBtnState == ButtonState::Release ||
                           AutoAdjustBtnState == ButtonState::MedRelease;
  if (!PxClkBtnPressed && !AutoAdjBtnPressed) {
    if (AdjPxClkMode != PxClkMode::None && AdjPxClkCnt-- == 0) {
      Buff.displayTxt("EXIT PxCLK", 0, 0, DISPLAY_TXT_ZOOM, /*Center=*/true);
      sleep_ms(PX_CLK_TXT_DISPLAY_MS);
      AdjPxClkMode = PxClkMode::None;
      return false;
    }
    return false;
  }
  AdjPxClkCnt = ADJUST_PX_CLK_CNT;
  bool SmallStep = AutoAdjustBtnState == ButtonState::MedRelease ||
                   PxClkBtnState == ButtonState::MedRelease;
  changePxClk(/*Increase=*/PxClkBtnPressed, SmallStep);
  getDividerAutomatically();
  switchPio();
  return true;
}

void TTLReader::runForEver() {
  while (true) {
    absolute_time_t FrameBegin = get_absolute_time();
    bool VSyncBool = Timing[Buff.getMode()][V_SyncPolarity] == Pos;
    auto M = Buff.getMode();
    // Wait here if we are in VSync retrace.
    bool RetraceVSync = Timing[M][V_SyncPolarity] == Pos;
    while (gpio_get(TTL_VSYNC_GPIO) == RetraceVSync)
      ;
    // A fresh frame, start with Line 0
    uint32_t Line = 0;
    auto Mode = Buff.getMode();
    while (true) {
      bool InVSync = false;
      switch (Mode) {
      case CGA_640x200_60Hz:
      case EGA_640x200_60Hz:
        InVSync = readLineCGA<CGA_640x200_60Hz>(Line);
        AutoAdjust.collect(TTLBorderPio, TTLBorderSM, EGABorderCounter, Line,
                           Mode);
        break;
      case EGA_640x350_60Hz:
        InVSync = readLineCGA<EGA_640x350_60Hz>(Line);
        AutoAdjust.collect(TTLBorderPio, TTLBorderSM, CGABorderCounter, Line,
                           Mode);
        break;
      case MDA_720x350_50Hz:
        InVSync = readLineMDA(Line);
        AutoAdjust.collect(TTLBorderPio, TTLBorderSM, MDABorderCounter, Line,
                           Mode);
        break;
      default:
        DBG_PRINT(std::cout << "Bad mode: " << modeToStr(Buff.getMode())
                            << "\n";)
        sleep_ms(1000);
      }
      ++Line;
      DBG_PRINT(MaxY = std::max(MaxY, Line);)
      if (InVSync)
        break;
    }
    MinY = std::min(MinY, MaxY);

    bool BordersAdjusted = AutoAdjust.frameTick(Mode);
    if (BordersAdjusted)
      saveToFlash();

    AutoAdjustBtn.tick();
    PxClkBtn.tick();

    if (AdjPxClkMode == PxClkMode::None)
      AutoAdjust.tryPushButtonStart();

    absolute_time_t FrameEnd = get_absolute_time();
    int64_t FrameUs = absolute_time_diff_us(FrameBegin, FrameEnd);

    bool Changed = tryChangePixelClock();
    if (!Changed) {
      static unsigned Cnt;
      if (++Cnt % 8 == 0) {
        bool VSyncPolarity = getVSyncPolarity();
        if (Buff.checkAndUpdateMode(FrameUs, FrameCnt, VSyncPolarity)) {
          getDividerAutomatically();
          switchPio();
        }
      }
    }

    // Try set the border from flash values.
    switch(Mode) {
    case CGA_640x200_60Hz:
    case EGA_640x200_60Hz:
      if (CGABorderOpt)
        AutoAdjust.setBorder(*CGABorderOpt);
      break;
    case EGA_640x350_60Hz:
      if (EGABorderOpt)
        AutoAdjust.setBorder(*EGABorderOpt);
      break;
    case MDA_720x350_50Hz:
      if (MDABorderOpt)
        AutoAdjust.setBorder(*MDABorderOpt);
      break;
    }

    // static uint32_t Cnt;
    // if (++Cnt % 128 == 0)
    //   DBG_PRINT(std::cout << " MaxY=" << MaxY << " MaxX=" << MaxX
    //                       << " Polarity="
    //                       << (getVSyncPolarity() == Pos ? "Pos" : "Neg")
    //                       << " Hz=" << DisplayBuffer::getHz(FrameUs) << "\n";)

    // DBG_PRINT(std::cout << std::hex << "VSyncBool=" << VSyncBool << std::dec
    // //                     << "\n";)
    // DBG_PRINT(std::cout << "XBorder=" << XBorder << " YBorder=" << YBorder
    //                     << " RawMin=" << RawMin << " RawMax=" << RawMax
    //                     << " MinY=" << MinY << " MaxY=" << MaxY << "\n";)
    DBG_PRINT(MaxX = 0;)
    DBG_PRINT(MaxY = 0;)
    MinY = std::numeric_limits<uint32_t>::max();
  }
}
