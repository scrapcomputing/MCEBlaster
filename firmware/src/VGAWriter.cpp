//-*- C++ -*-
//
// Copyright (C) 2024 Scrap Computing
//

#include "VGAWriter.h"
#include "NoInputSignal.pio.h"
#include "VGAOut4x1Pixels.pio.h"
#include "VGAOut8x1MDA.pio.h"
#include <config.h>
#include <pico/multicore.h>
#include <unordered_map>

DisplayBuffer Buff;

void VGAWriter::DrawBlackLineWithMask4x1(bool InVertSync) {
  static constexpr auto M = VGA_640x400_70Hz;

  static constexpr bool HPolarity = Timing[M][H_SyncPolarity];
  static constexpr bool VPolarity = Timing[M][V_SyncPolarity];
  auto Black4_Main = Black_4;
  if constexpr (HPolarity == Neg)
    Black4_Main |= HMask_4;
  if ((VPolarity == Neg && !InVertSync) || (VPolarity == Pos && InVertSync))
    Black4_Main |= VMask_4;

  for (unsigned i = 0; i < Timing[M][H_FrontPorch] + Timing[M][H_Visible] +
                               Timing[M][H_BackPorch];
       i += 4)
    pio_sm_put_blocking(VGAPio, VGASM, Black4_Main);

  auto Black4_InHSync = Black_4;
  if constexpr (HPolarity == Pos)
    Black4_InHSync |= HMask_4;
  if ((VPolarity == Neg && !InVertSync) || (VPolarity == Pos && InVertSync))
    Black4_InHSync |= VMask_4;
  for (unsigned i = 0; i < Timing[M][H_Sync]; i += 4)
    pio_sm_put_blocking(VGAPio, VGASM, Black4_InHSync);
}

void VGAWriter::DrawLineVSyncHigh4x1(unsigned Line) {
  static constexpr auto M = VGA_640x400_70Hz;
  // VSync is High throughout.
  // HSync is High for the boarders + visible parts.

  static constexpr bool HPolarity = Timing[M][H_SyncPolarity];
  static constexpr bool VPolarity = Timing[M][V_SyncPolarity];
  auto Black4_Porch = Black_4;
  if constexpr (HPolarity == Neg)
    Black4_Porch |= HMask_4;
  if constexpr (VPolarity == Neg)
    Black4_Porch |= VMask_4;

  // Front Porch is black.
  for (unsigned i = 0; i < Timing[M][H_FrontPorch]; i += 4)
    pio_sm_put_blocking(VGAPio, VGASM, Black4_Porch);

  // The visible part of the line.
  for (unsigned i = 0; i < Timing[M][H_Visible]; i += 4) {
    uint32_t Pix4 = Buff.get32(Line, i);
    if constexpr (HPolarity == Neg)
      Pix4 |= HMask_4;
    if constexpr (VPolarity == Neg)
      Pix4 |= VMask_4;
    pio_sm_put_blocking(VGAPio, VGASM, Pix4);
  }

  // Back Porch is black
  for (unsigned i = 0; i < Timing[M][H_BackPorch]; i += 4)
    pio_sm_put_blocking(VGAPio, VGASM, Black4_Porch);

  // Sync.
  auto Black4_Sync = Black_4;
  if constexpr (HPolarity == Pos)
    Black4_Sync |= HMask_4;
  if constexpr (VPolarity == Neg)
    Black4_Sync |= VMask_4;

  for (unsigned i = 0; i != Timing[M][H_Sync]; i += 4)
    pio_sm_put_blocking(VGAPio, VGASM, Black4_Sync);
}

void VGAWriter::DrawBlackLineWithMaskMDA8x1(uint32_t Mask_8) {
  static constexpr auto M = VGA_800x600_56Hz;
  const uint32_t BlackMDA_8_HM = BlackMDA_8 | HMaskMDA_8 | Mask_8;
  for (unsigned i = 0; i < Timing[M][H_FrontPorch] + Timing[M][H_Visible] +
                               Timing[M][H_BackPorch];
       i += 8)
    pio_sm_put_blocking(VGAPio, VGASM, BlackMDA_8_HM);

  const uint32_t BlackMDA_8_M = BlackMDA_8 | Mask_8;
  for (unsigned i = 0; i < Timing[M][H_Sync]; i += 8)
    pio_sm_put_blocking(VGAPio, VGASM, BlackMDA_8_M);
}

void VGAWriter::DrawLineVSyncHighMDA8x1(unsigned Line) {
  static constexpr auto M = VGA_800x600_56Hz;
  // VSync is High throughout.
  // HSync is High for the boarders + visible parts.
  // Front Porch
  for (unsigned i = 0; i < Timing[M][H_FrontPorch]; i += 8)
    pio_sm_put_blocking(VGAPio, VGASM, BlackMDA_8_HV);

  // Centering Margins
  static constexpr uint32_t MarginY =
    (Timing[M][V_Visible] - Timing[MDA_720x350_50Hz][V_Visible]) / 2;

  if (Line >= MarginY && Line < Timing[MDA_720x350_50Hz][V_Visible] + MarginY) {
    uint32_t BufferLine = Line - MarginY;
    // Not sure why, but if we print 720 pixels here we get a cropped image.
    // So instead we are printing the full 800 pixels.
    for (unsigned X = 0, E = Timing[M][H_Visible]; X < E; X += 8) {
      uint32_t Pixels8_HV = Buff.getMDA32(BufferLine, X) | HVMaskMDA_8;
      pio_sm_put_blocking(VGAPio, VGASM, Pixels8_HV);
    }
  } else {
    for (unsigned X = 0, E = Timing[M][H_Visible]; X < E; X += 8) {
      pio_sm_put_blocking(VGAPio, VGASM, BlackMDA_8_HV);
    }
  }

  // Right Border is black
  for (unsigned i = 0; i < Timing[M][H_BackPorch]; i += 8)
    pio_sm_put_blocking(VGAPio, VGASM, BlackMDA_8_HV);

  // Retrace: HSync is Low.
  for (unsigned i = 0; i < Timing[M][H_Sync]; i += 8)
    pio_sm_put_blocking(VGAPio, VGASM, BlackMDA_8_V);
}

void VGAWriter::tryChangePIOMode() {
  auto CurrMode = Buff.getMode();
  if (CurrMode == LastMode)
    return;
  LastMode = CurrMode;
  DBG_PRINT(std::cout << "VGAWriter: Change PIO Mode: " << modeToStr(CurrMode)
                      << "\n";)
  switch (CurrMode) {
  case CGA_640x200_60Hz:
  case EGA_640x200_60Hz:
  case EGA_640x350_60Hz:
    VGAOffset = PioLoader.loadPIOProgram(
        VGAPio, VGASM, &VGAOut4x1Pixels_program,
        [](PIO Pio, uint SM, uint Offset) {
          VGAOut4x1PixelsPioConfig(Pio, SM, Offset, VGA_RGB_GPIO);
        });
    pio_sm_put_blocking(VGAPio, VGASM, VGADarkYellow);
    pio_sm_put_blocking(VGAPio, VGASM, VGABrown);
    break;
  case MDA_720x350_50Hz:
    VGAOffset = PioLoader.loadPIOProgram(VGAPio, VGASM, &VGAOut8x1MDA_program,
                                         [](PIO Pio, uint SM, uint Offset) {
                                           VGAOut8x1MDAPioConfig(
                                               Pio, SM, Offset, VGA_RGB_GPIO);
                                         });
    break;
  default:
    DBG_PRINT(std::cout << "ERROR: no mode found!\n";)
    break;
  }
}

extern critical_section UnusedPIOLock;

VGAWriter::VGAWriter(Pico &Pico, PioProgramLoader &PioLoader)
    : PioLoader(PioLoader) {
  // Required for when the other core is writing to flash.
  multicore_lockout_victim_init();

  // TODO: The lock should not be needed since we are using a different Pio.
  critical_section_enter_blocking(&UnusedPIOLock);
  VGASM = pio_claim_unused_sm(VGAPio, true);
  NoInputSignalSM = pio_claim_unused_sm(NoInputSignalPio, true);
  critical_section_exit(&UnusedPIOLock);

  NoInputSignalOffset =
      pio_add_program(NoInputSignalPio, &NoInputSignal_program);
  noInputSignalPioConfig(NoInputSignalPio, NoInputSignalSM,
                         NoInputSignalOffset, TTL_HSYNC_GPIO);
  pio_sm_set_enabled(NoInputSignalPio, NoInputSignalSM, true);
  // Start VGA Pio
  tryChangePIOMode();
#ifndef DISALBE_PICO_LED
  Pico.ledON();
#endif
}

void VGAWriter::checkInputSignal() {
  auto Pio = NoInputSignalPio;
  auto SM = NoInputSignalSM;
  check_pio_param(Pio);
  check_sm_param(SM);
  if (pio_sm_is_rx_fifo_empty(Pio, SM)) {
    if (!NoSignal) {
      DBG_PRINT(if (!NoSignal) { std::cout << "No Signal\n"; })
      Buff.noSignal();
    }
    NoSignal = true;
    return;
  }
  DBG_PRINT(if (NoSignal) { std::cout << "End of No Signal\n"; })
  NoSignal = false;

  pio_sm_get(Pio, SM);
}

void VGAWriter::runForEver() {
  uint32_t Cnt = 0;
  while (true) {
    auto TTLMode = Buff.getMode();
    switch (TTLMode) {
    case CGA_640x200_60Hz:
    case EGA_640x200_60Hz:
    case EGA_640x350_60Hz: {
      static constexpr const auto M = VGA_640x400_70Hz;
      for (uint32_t Line = 0; Line != Timing[M][V_FrontPorch]; ++Line)
        DrawBlackLineWithMask4x1(/*InVSync=*/false);
      if (TTLMode == EGA_640x350_60Hz) {
        // 1. TTL Visible
        uint32_t Line = 0;
        uint32_t LineTTLE = Timing[TTLMode][V_Visible];
        for (; Line != LineTTLE; ++Line)
          DrawLineVSyncHigh4x1(std::min(Line, DisplayBuffer::BuffY - 1));
        // 2. The rest is black
        uint32_t LineVGAE = Timing[M][V_Visible];
        for (;Line != LineVGAE; ++Line)
          DrawBlackLineWithMask4x1(/*InVSync=*/false);
      } else {
        for (uint32_t Line = 0; Line != Timing[M][V_Visible]; ++Line)
          DrawLineVSyncHigh4x1(Line/2);
      }
      for (uint32_t Line = 0; Line != Timing[M][V_BackPorch]; ++Line)
        DrawBlackLineWithMask4x1(/*InVSync=*/false);
      for (uint32_t Line = 0; Line != Timing[M][V_Sync]; ++Line)
        DrawBlackLineWithMask4x1(/*InVSync=*/true);
      break;
    }
    case MDA_720x350_50Hz: {
      static constexpr const auto M = VGA_800x600_56Hz;
      for (uint32_t Line = 0; Line != Timing[M][V_FrontPorch]; ++Line)
        DrawBlackLineWithMaskMDA8x1(VMaskMDA_8);
      for (uint32_t Line = 0; Line != Timing[M][V_Visible]; ++Line)
        DrawLineVSyncHighMDA8x1(Line);
      for (uint32_t Line = 0; Line != Timing[M][V_BackPorch]; ++Line)
        DrawBlackLineWithMaskMDA8x1(VMaskMDA_8);
      for (uint32_t Line = 0; Line != Timing[M][V_Sync]; ++Line)
        DrawBlackLineWithMaskMDA8x1(BlackMDA_8);
      break;
    }
    default:
      DBG_PRINT(std::cout << "Unimplemented mode "
                          << modeToStr(TTLMode);)
      break;
    }
    // Update PIO if needed.
    tryChangePIOMode();

    if (++Cnt % NoSignalCheckFreq == 0)
      checkInputSignal();
  }
}
