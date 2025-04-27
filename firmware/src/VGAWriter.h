//-*- C++ -*-
//
// Copyright (C) 2024 Scrap Computing
//

#ifndef __VGAWRITER_H__
#define __VGAWRITER_H__

#include "DisplayBuffer.h"
#include "Pico.h"
#include "PioProgramLoader.h"
#include "Timings.h"
#include "hardware/pio.h"

extern DisplayBuffer Buff;

class VGAWriter {
  Pico &Pi;

  PIO VGAPio = pio1;
  uint VGASM = 0;
  uint VGAOffset = 0;

  PIO NoInputSignalPio = pio1;
  uint NoInputSignalSM = 0;
  uint NoInputSignalOffset = 0;

  const PIO VSyncPolarityPio = pio0;
  uint VSyncPolaritySM = 0;
  uint VSyncPolarityOffset = 0;

  const PIO HSyncPolarityPio = pio0;
  uint HSyncPolaritySM = 0;
  uint HSyncPolarityOffset = 0;

  Polarity VSyncPolarity = Polarity::Pos;
  Polarity HSyncPolarity = Polarity::Pos;

  TTLDescr LastMode;
  /// We deliberately maintain a copy of the TTLReader's mode rather than a
  /// pointer to avoid getting the mode updated while we are drawing on screen.
  TTLDescr TimingsTTL;
  PioProgramLoader &PioLoader;

  bool NoSignal = false;

  void checkInputSignal();

  void DrawBlackLineWithMask4x1(bool InVertSync);
  void DrawLineVSyncHigh4x1(unsigned Line);

  void DrawBlackLineWithMaskMDA8x1(uint32_t Mask_4);
  void DrawLineVSyncHighMDA8x1(unsigned Line);

  /// Starts/changes PIO program based on the Buffer's mode.
  void tryChangePIOMode();

  /// Print a single frame using the 4x1 routines.
  template <VGAResolution R, bool LineDoubling> void drawFrame4x1();

  /// Print a single frame using the 8x1 routines.
  template <VGAResolution R, bool LineDoubling> void drawFrame8x1();

  void restartCore1TTLReader(bool NoSignal);

public:
  VGAWriter(Pico &Pico, PioProgramLoader &PioLoader);
  void runForEver();
};

#endif // __VGAWRITER_H__
