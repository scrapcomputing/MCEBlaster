//-*- C++ -*-
//
// Copyright (C) 2024 Scrap Computing
//

#ifndef __VGAWRITER_H__
#define __VGAWRITER_H__

#include "Pico.h"
#include "DisplayBuffer.h"
#include "TimingsVGA.h"
#include "hardware/pio.h"
#include "PioProgramLoader.h"

extern DisplayBuffer Buff;

class VGAWriter {
  PIO VGAPio = pio1;
  uint VGASM = 0;
  uint VGAOffset = 0;

  PIO NoInputSignalPio = pio1;
  uint NoInputSignalSM = 0;
  uint NoInputSignalOffset = 0;
  Resolution LastMode = RES_MAX;
  PioProgramLoader &PioLoader;

  bool NoSignal = false;

  void checkInputSignal();

  void DrawBlackLineWithMask4x1(bool InVertSync);
  void DrawLineVSyncHigh4x1(unsigned Line);

  void DrawBlackLineWithMaskMDA8x1(uint32_t Mask_4);
  void DrawLineVSyncHighMDA8x1(unsigned Line);

  /// Starts/changes PIO program based on the Buffer's mode.
  void tryChangePIOMode();

public:
  VGAWriter(Pico &Pico, PioProgramLoader &PioLoader);
  void runForEver();
};

#endif // __VGAWRITER_H__
