//-*- C++ -*-
//
// Copyright (C) 2024 Scrap Computing
//

#ifndef __TTLREADER_H__
#define __TTLREADER_H__

#include "Button.h"
#include "CGA640x200Border.pio.h"
#include "CGAPio.h"
#include "ClkDivider.h"
#include "Common.h"
#include "EGA640x350Border.pio.h"
#include "EGAPio.h"
#include "Flash.h"
#include "MDA720x350Border.pio.h"
#include "MDAPio.h"
#include "PioProgramLoader.h"
#include "VSyncPolarity.pio.h"
#include "TimingsVGA.h"
#include "hardware/pio.h"
#include <limits>

using ButtonTy = Button</*OffVal=*/true, /*DebounceSz=*/2,
                        /*LongPressCntVal=*/BTN_LONG_PRESS_FRAMES>;

/// Helper class for the border x,y data.
class BorderXY {
public:
  BorderXY(uint32_t X, uint32_t Y) : X(X), Y(Y) {}
  BorderXY(uint32_t XY) {
    X = XY & 0xffff;
    Y = (XY >> 16) & 0xffff;
  }
  uint16_t X;
  uint16_t Y;
  uint32_t getUint32() const {
    uint32_t Val = X;
    uint32_t Y32 = Y;
    Val |= (Y32 << 16);
    return Val;
  }
};

class AutoAdjustBorder {
  // These are references to TTLReader values.
  uint32_t &XBorder;
  uint32_t &YBorder;
  std::optional<BorderXY> &CGABorderOpt;
  std::optional<BorderXY> &EGABorderOpt;
  std::optional<BorderXY> &MDABorderOpt;


  uint32_t TmpXBorder = 0;
  uint32_t TmpYBorder = 0;

  enum class State {
    Off,
    SingleON,
    AlwaysON,
  };

  State Enabled = State::Off;
  uint32_t StartFrame = 0;
  uint32_t StopFrame = 0;
  uint32_t FrameCnt = 0;
  uint32_t ThrottleCnt = 0;
  ButtonTy &Btn;
  FlashStorage &Flash;
  static constexpr const uint32_t XBorderInit = 164;
  static constexpr const uint32_t YBorderInit = 64;
  inline void resetBorders();
  inline void applyBorders(Resolution Mode);

public:
  AutoAdjustBorder(uint32_t &XBorder, uint32_t &YBorder,
                   std::optional<BorderXY> &CGABorderOpt,
                   std::optional<BorderXY> &EGABorderOpt,
                   std::optional<BorderXY> &MDABorderOpt, ButtonTy &Btn,
                   FlashStorage &Flash)
      : XBorder(XBorder), YBorder(YBorder), CGABorderOpt(CGABorderOpt),
        EGABorderOpt(EGABorderOpt), MDABorderOpt(MDABorderOpt), Btn(Btn),
        Flash(Flash) {}
  void forceStart(bool AlwaysON);
  void tryPushButtonStart();
  void collect(PIO Pio, uint32_t SM, uint32_t ModeBorderCounter, uint32_t Line,
               Resolution M);
  /// Gets called on every frame. Returns if borders were applied.
  bool frameTick(Resolution Mode);
  bool isAlwaysON() const { return Enabled == State::AlwaysON; }
  /// Used to set the borders when stored in flash.
  void setBorder(const BorderXY &XY);
};

class TTLReader {
  /// This unit reads the TTL video signal from the video card.
  PIO TTLPio;
  uint TTLSM;
  uint TTLOffset;

  /// This unit helps find the visible border.
  PIO TTLBorderPio;
  uint TTLBorderSM;
  uint TTLBorderOffset;

  PIO VSyncPolarityPio;
  uint VSyncPolaritySM;
  uint VSyncPolarityOffset;

  static constexpr const uint32_t VSyncPolarityCounter =
      std::numeric_limits<uint32_t>::max();

  uint32_t MaxY = 0;            // For debugging
  uint32_t MinY = std::numeric_limits<uint32_t>::max();
  uint32_t MaxX = 0;            // For debugging

  uint32_t FrameCnt = 0;

  uint32_t XBorder = 0; // WARNING: Must be 4-byte aligned! (i.e. last 2 bits 0)
  uint32_t YBorder = 0;

  uint32_t EGAIPP = 0;
  uint32_t CGAIPP = 0;
  uint32_t MDAIPP = 0;

  ClkDivider CGAClkDiv{1, 66};
  ClkDivider EGAClkDiv{1, 131};
  ClkDivider MDAClkDiv{1, 66};

  // Flash entries
  enum {
    CGAPxClkIdx = 0,
    EGAPxClkIdx,
    MDAPxClkIdx,
    CGABorderIdx,
    EGABorderIdx,
    MDABorderIdx,
    MaxFlashIdx,
  };
  static constexpr const uint32_t InvalidBorder = 0xffffffff;

  uint32_t CGAPxClk = Timing[CGA_640x200_60Hz][PxClk];
  uint32_t EGAPxClk = Timing[EGA_640x350_60Hz][PxClk];
  uint32_t MDAPxClk = Timing[MDA_720x350_50Hz][PxClk];

  std::optional<BorderXY> CGABorderOpt;
  std::optional<BorderXY> EGABorderOpt;
  std::optional<BorderXY> MDABorderOpt;

  uint32_t &getPxClkFor(Resolution M);
  /// Increments the clock divider that corresponds to the current mode.
  void changePxClk(bool Increase, bool SmallStep);

  ButtonTy AutoAdjustBtn;
  ButtonTy PxClkBtn;

  enum class PxClkMode {
    None,
    Modify,
    Reset,
  };
  PxClkMode AdjPxClkMode = PxClkMode::None;

  AutoAdjustBorder AutoAdjust;
  uint32_t AdjPxClkCnt = ADJUST_PX_CLK_CNT;

  static constexpr const uint32_t TimingNOPsCGA = 7;
  static constexpr const uint32_t TimingNOPsMDA = 7;

  PioProgramLoader &PioLoader;

  FlashStorage &Flash;

  template <uint32_t M>
  inline bool readLineCGA(uint32_t Line);
  inline bool readLineMDA(uint32_t Line);
  /// Updates the PIO's timing NOPs based on the current display mode.
  void setTimingNOPs() ;

  /// \Returns true if we have read a valid border for mode \p M from flash.
  bool haveBorderFromFlash(Resolution M) const;

  void switchPio();

  void getDividerAutomatically();

  bool getVSyncPolarity() const;

  /// Save settings to flash.
  void saveToFlash();

  bool tryChangePixelClock();

public:
  TTLReader(PioProgramLoader &PioLoader, Pico &Pi, FlashStorage &Flash);
  void runForEver();
};

#endif // __TTLREADER_H__

