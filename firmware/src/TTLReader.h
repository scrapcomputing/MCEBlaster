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
#include "HorizMenu.h"
#include "MDA720x350Border.pio.h"
#include "MDAPio.h"
#include "PioProgramLoader.h"
#include "Timings.h"
#include "hardware/pio.h"
#include <limits>

class DisplayBuffer;

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

class TTLReader;

class AutoAdjustBorder {
  // These are references to TTLReader values.
  const bool &ManualTTLEnabled;
  const TTLDescrReduced &ManualTTL;
  const bool &XBorderAUTO;
  uint32_t &XBorder;
  const bool &YBorderAUTO;
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
  FlashStorage &Flash;
  static constexpr const uint32_t XBorderInit = 164;
  static constexpr const uint32_t YBorderInit = 64;
  inline void resetBorders();
  inline void applyBorders(const TTLDescr &TimingsTTL);
  TTLReader &TTLR;

public:
  AutoAdjustBorder(const bool &ManualTTLEnabled,
                   const TTLDescrReduced &ManualTTL, const bool &XBorderAUTO,
                   uint32_t &XBorder, const bool &YBorderAUTO,
                   uint32_t &YBorder, std::optional<BorderXY> &CGABorderOpt,
                   std::optional<BorderXY> &EGABorderOpt,
                   std::optional<BorderXY> &MDABorderOpt, FlashStorage &Flash,
                   TTLReader &TTLR)
      : ManualTTLEnabled(ManualTTLEnabled), ManualTTL(ManualTTL),
        XBorderAUTO(XBorderAUTO), XBorder(XBorder), YBorderAUTO(YBorderAUTO),
        YBorder(YBorder), CGABorderOpt(CGABorderOpt),
        EGABorderOpt(EGABorderOpt), MDABorderOpt(MDABorderOpt), Flash(Flash),
        TTLR(TTLR) {}
  void forceStart(bool AlwaysON);
  void runAutoAdjust();
  void runAlwaysAutoAdjust();
  void collect(PIO Pio, uint32_t SM, uint32_t ModeBorderCounter, uint32_t Line,
               TTL Mode);
  /// Gets called on every frame. Returns if borders were applied.
  bool frameTick(const TTLDescr &TimingsTTL);
  bool isAlwaysON() const { return Enabled == State::AlwaysON; }
  /// Used to set the borders when stored in flash.
  void setBorder(const BorderXY &XY);
};

class TTLReader {
  Pico &Pi;
  /// This unit reads the TTL video signal from the video card.
  PIO TTLPio;
  uint TTLSM;
  uint TTLOffset;

  /// This unit helps find the visible border.
  PIO TTLBorderPio;
  uint TTLBorderSM;
  uint TTLBorderOffset;
public:
  static constexpr const uint32_t SyncPolarityCounterInit = 31;

private:
  uint32_t FrameCnt = 0;

  TTLDescr TimingsTTL = PresetTimingsTTL[DisplayBufferDefaultTTL];

  // WARNING: Must be 4-byte aligned! (i.e. last 2 bits 0)
  bool XBorderAUTO = true;
  uint32_t &XBorder = TimingsTTL.H_FrontPorch;
  bool YBorderAUTO = true;
  uint32_t &YBorder = TimingsTTL.V_FrontPorch;

  uint32_t EGAIPP = 0;
  uint32_t CGAIPP = 0;
  uint32_t MDAIPP = 0;

  ClkDivider CGAClkDiv{1, 66};
  ClkDivider EGAClkDiv{1, 131};
  ClkDivider MDAClkDiv{1, 66};

  // Flash entries
  enum {
    CGAPxClkIdx = 0,
    CGASamplingOffsetIdx,
    EGAPxClkIdx,
    EGASamplingOffsetIdx,
    MDAPxClkIdx,
    MDASamplingOffsetIdx,
    CGABorderIdx,
    EGABorderIdx,
    MDABorderIdx,
    ManualTTL_EnabledIdx,
    ManualTTL_ModeIdx,
    ManualTTL_H_VisibleIdx,
    ManualTTL_V_VisibleIdx,
    XBorderAUTOIdx,
    ManualTTL_H_BackPorchIdx,
    YBorderAUTOIdx,
    ManualTTL_V_BackPorchIdx,
    MaxFlashIdx,
  };
  static constexpr const uint32_t InvalidBorder = 0xffffffff;

  uint32_t CGAPxClk = PresetTimingsTTL[CGA_640x200_60Hz].PxClk;
  uint32_t EGAPxClk = PresetTimingsTTL[EGA_640x350_60Hz].PxClk;
  uint32_t MDAPxClk = PresetTimingsTTL[MDA_720x350_50Hz].PxClk;

  uint32_t CGASamplingOffset = 0;
  uint32_t EGASamplingOffset = 0;
  uint32_t MDASamplingOffset = 0;

  std::optional<BorderXY> CGABorderOpt;
  std::optional<BorderXY> EGABorderOpt;
  std::optional<BorderXY> MDABorderOpt;

  uint32_t &getPxClkFor(const TTLDescr &Descr);
  uint32_t &getSamplingOffsetFor(const TTLDescr &Descr);
  void displayPxClk();
  /// Increments the clock divider that corresponds to the current mode.
  void changePxClk(bool Increase, bool SmallStep);

  PioProgramLoader &PioLoader;

  ButtonTy AutoAdjustBtn;
  ButtonTy PxClkBtn;

  enum class UserAction {
    None,
    PxClkMode_Modify,
    TTLInfo,
    ManualTTL,
  };
  // We use this action state variable for all actions (like, manualTTL,
  // auto-adjust, pxClock) to make sure we are not servicing more than one
  // action at once.
  UserAction UsrAction = UserAction::None;
  bool AllowEnterMenu = false;
  std::optional<absolute_time_t> ManualTTLExitTime;

  AutoAdjustBorder AutoAdjust;
  std::optional<absolute_time_t> PxClkEndTime;
  bool ChangedPxClk = false;

  /// TTL mode forced by the user. This disables auto-detection.
  TTLDescrReduced ManualTTL;
  bool ManualTTLEnabled = false;

  static constexpr const uint32_t TimingNOPsCGA = 7;
  static constexpr const uint32_t TimingNOPsMDA = 7;

  FlashStorage &Flash;

  template <bool DiscardData> inline bool readLineCGA(uint32_t Line);
  template <bool DiscardData> inline bool readLineMDA(uint32_t Line);
  /// Updates the PIO's timing NOPs based on the current display mode.
  void setTimingNOPs() ;

  /// \Returns true if we have read a valid border for the current TimingsTTL
  /// from flash.
  bool haveBorderFromFlash() const;

  void switchPio();

  void getDividerAutomatically();

  Polarity &VSyncPolarity = TimingsTTL.V_SyncPolarity;
  Polarity &HSyncPolarity = TimingsTTL.H_SyncPolarity;

  PIO VSyncPolarityPio;
  int VSyncPolaritySM;
  PIO HSyncPolarityPio;
  int HSyncPolaritySM;

  /// Save settings to flash.
  void saveToFlash();

  void toggleManualTTL();
  void printManualTTLMenu();

  /// Checks if the user has pushed the buttons to force a specific TTL mode.
  /// UsrAction is set to ManualTTL while we are servicing this request.
  bool manualTTLMode();

  /// This is used to avoid hanging while reading TTL when TTL is not connected.
  /// In this way we can still have functional menus while TTL is down.
  bool NoSignal = true;
  bool LastNoSignal = false;

  std::vector<std::pair<PIO, uint>> UsedSMs;
  /// \Reutrns an unused SM but also pushes it into `UnusedSMs` for unclaiming
  /// them before a restart of the TTLReader.
  int claimUnusedSMSafe(PIO Pio);

  /// The vertical sync frequency.
  float &VHz = TimingsTTL.V_Hz;
  /// The horizontal
  float &HHz = TimingsTTL.H_Hz;
  absolute_time_t FrameEnd;
  /// Limits the number of "UNKNOWN MODE" messages the user will see.
  int UnknownMsgMaxCnt = UNKNOWN_MODE_SHOW_MSG_MAX_COUNT;
  /// Wait for a bit until we display the message to avoid false messages.
  int UnknownMsgMinCnt = UNKNOWN_MODE_SHOW_MSG_MIN_COUNT;
  void checkAndUpdateMode();
  /// Display a screen with information about the TTL signal.
  void displayTTLInfo();
  /// Take actions based on button state.
  void handleButtons();
  /// Calculates the polarity for both V and H sync based on the PIO values and
  /// updates VSyncPolarity and HSyncPolarity variables.
  void updateSyncPolarityVariables();

  bool ResetToDefaults;

  DisplayBuffer &Buff;

  template <TTL M, bool DiscardLineData> bool readLinePerMode(uint32_t Line);
  template <TTL M> void readFrame(uint32_t &Line);

public:
  DisplayBuffer &getBuff() { return Buff; }
  /// \Returns true if \p Descr is high resolution. This is to tell apart EGA
  /// 640x350 from 640x200.
  static bool isHighRes(const TTLDescr &Descr);

  const TTLDescr &getMode() const { return TimingsTTL; }
  void unclaimUsedSMs();

  std::optional<absolute_time_t> DisplayTxtEndTime;
  /// NOTE: This won't write to the display directly. You need to call
  /// displayTxtTick()!
  void displayTxt(const std::string &Txt, int Time = 0);
  /// Copies the TxtBuffer to the visible Buffer.
  void displayTxtTick();
  int ManualTTLMenuIdx = 0;
  static constexpr const int ManualTTLMenu_Enabled_ItemIdx = 0;
  static constexpr const int ManualTTLMenu_Mode_ItemIdx = 1;
  static constexpr const int ManualTTLMenu_Horiz_ItemIdx = 2;
  static constexpr const int ManualTTLMenu_Vert_ItemIdx = 3;
  static constexpr const int ManualTTLMenu_XBorderAUTO_ItemIdx = 4;
  static constexpr const int ManualTTLMenu_XBorder_ItemIdx = 5;
  static constexpr const int ManualTTLMenu_YBorderAUTO_ItemIdx = 6;
  static constexpr const int ManualTTLMenu_YBorder_ItemIdx = 7;
  static constexpr const int ManualTTLMenu_NumMenuItems = 8;

  HorizMenu ManualTTLMenu;

  TTLReader(PioProgramLoader &PioLoader, Pico &Pi, FlashStorage &Flash,
            DisplayBuffer &Buff, PIO VSyncPolarityPio, uint VSyncPolaritySM,
            PIO HSyncPolarityPio, uint HSyncPolaritySM, bool ResetToDefaults);
  void setBorders();
  absolute_time_t FrameBegin;
  void calculateVHSyncs(absolute_time_t LastFrameEnd, uint32_t VisibleLines);
  void runForEver();
  void setNoSignal(bool Val) {
    DBG_PRINT(std::cout << "TTLReader: setNoSignal=" << Val << "\n";)
    NoSignal = Val;
  }
};

#endif // __TTLREADER_H__

