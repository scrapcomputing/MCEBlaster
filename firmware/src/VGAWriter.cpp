//-*- C++ -*-
//
// Copyright (C) 2024 Scrap Computing
//

#include "VGAWriter.h"
#include "HSyncPolarity.pio.h"
#include "NoInputSignal.pio.h"
#include "TTLReader.h"
#include "VGAOut4x1Pixels.pio.h"
#include "VGAOut8x1MDA.pio.h"
#include "VSyncPolarity.pio.h"
#include <config.h>
#include <pico/multicore.h>
#include <unordered_map>

static bool ResetToDefaults = false;
DisplayBuffer Buff;
static TTLReader *TTLReaderPtr = nullptr;
extern PioProgramLoader *PPL;
extern Pico *Pi;
extern FlashStorage *Flash;

static PIO VSyncPolarityPio_ = 0;
static int VSyncPolaritySM_ = 0;
static PIO HSyncPolarityPio_ = 0;
static int HSyncPolaritySM_ = 0;

void __not_in_flash_func(VGAWriter::DrawBlackLineWithMask4x1)(bool InVertSync) {
  static constexpr auto M = VGA_640x400_70Hz;

  static constexpr Polarity HPolarity = TimingsVGA[M].H_SyncPolarity;
  static constexpr Polarity VPolarity = TimingsVGA[M].V_SyncPolarity;
  auto Black4_Main = Black_4;
  if constexpr (HPolarity == Neg)
    Black4_Main |= HMask_4;
  if ((VPolarity == Neg && !InVertSync) || (VPolarity == Pos && InVertSync))
    Black4_Main |= VMask_4;

  for (unsigned i = 0;
       i < TimingsVGA[M].H_BackPorch + TimingsVGA[M].H_Visible +
               TimingsVGA[M].H_FrontPorch;
       i += 4)
    pio_sm_put_blocking(VGAPio, VGASM, Black4_Main);

  auto Black4_InHSync = Black_4;
  if constexpr (HPolarity == Pos)
    Black4_InHSync |= HMask_4;
  if ((VPolarity == Neg && !InVertSync) || (VPolarity == Pos && InVertSync))
    Black4_InHSync |= VMask_4;
  for (unsigned i = 0; i < TimingsVGA[M].H_Retrace; i += 4)
    pio_sm_put_blocking(VGAPio, VGASM, Black4_InHSync);
}

void __not_in_flash_func(VGAWriter::DrawLineVSyncHigh4x1)(unsigned Line) {
  static constexpr auto M = VGA_640x400_70Hz;
  // VSync is High throughout.
  // HSync is High for the boarders + visible parts.

  static constexpr Polarity HPolarity = TimingsVGA[M].H_SyncPolarity;
  static constexpr Polarity VPolarity = TimingsVGA[M].V_SyncPolarity;
  auto Black4_Porch = Black_4;
  if constexpr (HPolarity == Polarity::Neg)
    Black4_Porch |= HMask_4;
  if constexpr (VPolarity == Polarity::Neg)
    Black4_Porch |= VMask_4;

  // Back Porch is black.
  for (unsigned i = 0; i < TimingsVGA[M].H_BackPorch; i += 4)
    pio_sm_put_blocking(VGAPio, VGASM, Black4_Porch);

  // The visible part of the line.
  for (unsigned i = 0; i < TimingsVGA[M].H_Visible; i += 4) {
    uint32_t Pix4 = Buff.get32(Line, i);
    if constexpr (HPolarity == Neg)
      Pix4 |= HMask_4;
    if constexpr (VPolarity == Neg)
      Pix4 |= VMask_4;
    pio_sm_put_blocking(VGAPio, VGASM, Pix4);
  }

  // Front Porch is black
  for (unsigned i = 0; i < TimingsVGA[M].H_FrontPorch; i += 4)
    pio_sm_put_blocking(VGAPio, VGASM, Black4_Porch);

  // Sync.
  auto Black4_Sync = Black_4;
  if constexpr (HPolarity == Pos)
    Black4_Sync |= HMask_4;
  if constexpr (VPolarity == Neg)
    Black4_Sync |= VMask_4;

  for (unsigned i = 0; i != TimingsVGA[M].H_Retrace; i += 4)
    pio_sm_put_blocking(VGAPio, VGASM, Black4_Sync);
}

void __not_in_flash_func(VGAWriter::DrawBlackLineWithMaskMDA8x1)(uint32_t Mask_8) {
  static constexpr auto M = VGA_800x600_56Hz;
  const uint32_t BlackMDA_8_HM = BlackMDA_8 | HMaskMDA_8 | Mask_8;
  for (unsigned i = 0;
       i < TimingsVGA[M].H_BackPorch + TimingsVGA[M].H_Visible +
               TimingsVGA[M].H_FrontPorch;
       i += 8)
    pio_sm_put_blocking(VGAPio, VGASM, BlackMDA_8_HM);

  const uint32_t BlackMDA_8_M = BlackMDA_8 | Mask_8;
  for (unsigned i = 0; i < TimingsVGA[M].H_Retrace; i += 8)
    pio_sm_put_blocking(VGAPio, VGASM, BlackMDA_8_M);
}

void __not_in_flash_func(VGAWriter::DrawLineVSyncHighMDA8x1)(unsigned Line) {
  static constexpr auto M = VGA_800x600_56Hz;
  // VSync is High throughout.
  // HSync is High for the boarders + visible parts.
  // Back Porch
  unsigned X = 0;
  for (; X < TimingsVGA[M].H_BackPorch; X += 8)
    pio_sm_put_blocking(VGAPio, VGASM, BlackMDA_8_HV);

  // Visible TTL is 720 pixels but VGA is 800 so we need to pad with black
  // pixels such that the image can get centered proplerly.
  unsigned Padding = (TimingsVGA[M].H_Visible - TimingsTTL.H_Visible) / 2;
  for (; X < Padding; X += 8)
    pio_sm_put_blocking(VGAPio, VGASM, BlackMDA_8_HV);

  // The visible part of the line.
  for (unsigned Idx = 0, E = TimingsTTL.H_Visible; Idx < E; Idx += 8) {
    uint32_t Pixels8_HV = Buff.getMDA32(Line, Idx) | HVMaskMDA_8;
    pio_sm_put_blocking(VGAPio, VGASM, Pixels8_HV);
  }
  X += TimingsTTL.H_Visible;

  // Fill the right hand side Visible TTL-VGA gap and the front-porch with black
  // pixels.
  for (unsigned E = TimingsVGA[M].H_BackPorch + TimingsVGA[M].H_Visible +
                    TimingsVGA[M].H_FrontPorch;
       X < E; X += 8)
    pio_sm_put_blocking(VGAPio, VGASM, BlackMDA_8_HV);

  // Retrace: HSync is Low.
  for (unsigned i = 0; i < TimingsVGA[M].H_Retrace; i += 8)
    pio_sm_put_blocking(VGAPio, VGASM, BlackMDA_8_V);
}

void VGAWriter::tryChangePIOMode() {
  TimingsTTL = TTLReaderPtr->getMode();

  if (TimingsTTL == LastMode)
    return;
  LastMode = TimingsTTL;
  Buff.setMode(TimingsTTL);
  DBG_PRINT(std::cout << "VGAWriter: Change PIO Mode: "
                      << modeToStr(TimingsTTL.Mode) << "\n";)
  switch (TimingsTTL.Mode) {
  case TTL::CGA:
  case TTL::EGA:
    VGAOffset = PioLoader.loadPIOProgram(
        VGAPio, VGASM, &VGAOut4x1Pixels_program,
        [](PIO Pio, uint SM, uint Offset) {
          VGAOut4x1PixelsPioConfig(Pio, SM, Offset, VGA_RGB_GPIO);
        });
    pio_sm_put_blocking(VGAPio, VGASM, VGADarkYellow);
    pio_sm_put_blocking(VGAPio, VGASM, VGABrown);
    break;
  case TTL::MDA:
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
  DBG_PRINT(std::cout << "VGAWriter:: done changing PIO\n";)
}

VGAWriter::VGAWriter(Pico &Pico, PioProgramLoader &PioLoader)
    : Pi(Pico), PioLoader(PioLoader) {
  // Required for when the other core is writing to flash.
  multicore_lockout_victim_init();

  Buff.clear();
  Buff.setMode(TimingsTTL);

  VGASM = pio_claim_unused_sm(VGAPio, true);

  NoInputSignalSM = pio_claim_unused_sm(NoInputSignalPio, true);
  NoInputSignalOffset =
      pio_add_program(NoInputSignalPio, &NoInputSignal_program);
  noInputSignalPioConfig(NoInputSignalPio, NoInputSignalSM,
                         NoInputSignalOffset, TTL_HSYNC_GPIO);
  pio_sm_set_enabled(NoInputSignalPio, NoInputSignalSM, true);

  // Start the VSyncPolarity PIO.
  VSyncPolaritySM = pio_claim_unused_sm(VSyncPolarityPio, true);
  VSyncPolarityOffset =
      pio_add_program(VSyncPolarityPio, &VSyncPolarity_program);
  VSyncPolarityPioConfig(VSyncPolarityPio, VSyncPolaritySM, VSyncPolarityOffset,
                         TTL_VSYNC_GPIO);
  pio_sm_set_enabled(VSyncPolarityPio, VSyncPolaritySM, true);
  DBG_PRINT(std::cout << "Started VSyncPolarty Pio\n";)

  // Start the HSyncPolarity PIO.
  HSyncPolaritySM = pio_claim_unused_sm(HSyncPolarityPio, true);
  HSyncPolarityOffset =
      pio_add_program(HSyncPolarityPio, &HSyncPolarity_program);
  HSyncPolarityPioConfig(HSyncPolarityPio, HSyncPolaritySM, HSyncPolarityOffset,
                         TTL_HSYNC_GPIO);
  pio_sm_set_enabled(HSyncPolarityPio, HSyncPolaritySM, true);
  DBG_PRINT(std::cout << "Started HSyncPolarty Pio\n";)

  // Reset to defaults if the user is pressinx PxClkBtn during boot.
  ResetToDefaults = !gpio_get(PX_CLK_BTN_GPIO) && !gpio_get(AUTO_ADJUST_GPIO);

  // Start TTLReader. We don't know yet the state of NoSignal, but it's fine.
  restartCore1TTLReader(NoSignal);

  // Start VGA Pio
  tryChangePIOMode();

#ifndef DISALBE_PICO_LED
  Pico.ledON();
#endif
}

static void core1_main() {
  TTLReader TTLR(*PPL, *Pi, *Flash, Buff, VSyncPolarityPio_, VSyncPolaritySM_,
                 HSyncPolarityPio_, HSyncPolaritySM_, ResetToDefaults);
  ResetToDefaults = false;      // Only once
  TTLReaderPtr = &TTLR;
  DBG_PRINT(std::cout << "TTLR started\n";)
  TTLR.runForEver();
}

void VGAWriter::restartCore1TTLReader(bool NoSignal) {
  // Restart core1 because it must have hanged waiting for TTL
  DBG_PRINT(std::cout << "Starting or restarting Core1\n";)
  if (TTLReaderPtr != nullptr) {
    DBG_PRINT(std::cout << "RESET CORE 1\n";)
    TTLReaderPtr->unclaimUsedSMs();
    multicore_reset_core1();
    // multicore_fifo_pop_blocking();
    sleep_ms(10);
  }
  TTLReaderPtr = nullptr;
  VSyncPolarityPio_ = VSyncPolarityPio;
  VSyncPolaritySM_ = VSyncPolaritySM;
  HSyncPolarityPio_ = HSyncPolarityPio;
  HSyncPolaritySM_ = HSyncPolaritySM;
  multicore_launch_core1(core1_main);

  DBG_PRINT(std::cout << "VGAWriter Wait for TTLReader\n";)
  while (TTLReaderPtr == nullptr)
    Utils::sleep_ms(50);
  TimingsTTL = TTLReaderPtr->getMode();
  TTLReaderPtr->setNoSignal(NoSignal);
  DBG_PRINT(std::cout << "VGAWriter Wait Done\n";)
}

void VGAWriter::checkInputSignal() {
  // If the HSync polarity PIO's buffer is empty then there is no TTL signal
  if (pio_sm_is_rx_fifo_empty(NoInputSignalPio, NoInputSignalSM)) {
    // No signal!
    if (!NoSignal) {
      DBG_PRINT(std::cout << "No Signal\n";)
      NoSignal = true;
      restartCore1TTLReader(NoSignal);
    }
    return;
  }
  // Yes signal!
  if (NoSignal) {
    DBG_PRINT(std::cout << "\n\nEnd of No Signal\n";)
    NoSignal = false;
    restartCore1TTLReader(NoSignal);
  }
  // Pop from the nosignal queue.
  pio_sm_get(NoInputSignalPio, NoInputSignalSM);
}

template <VGAResolution R, bool LineDoubling>
void __not_in_flash_func(VGAWriter::drawFrame4x1)() {
  // Back porch is black.
  for (uint32_t Line = 0; Line != TimingsVGA[R].V_BackPorch; ++Line)
    DrawBlackLineWithMask4x1(/*InVSync=*/false);

  // 1. TTL Visible.
  uint32_t Line = 0;
  // If there is empty space below the screen (e.g. in TTL resolutions like
  // 640x350 that are drawn in 640x400), then center the image vertically.
  uint32_t TTLV = (LineDoubling ? 2 : 1) * TimingsTTL.V_Visible;
  if (TimingsVGA[R].V_Visible > TTLV) {
    // Vertical VGA-TTL visible gap, fill with black lines.
    uint32_t VGA_TTL_VerticalGap = TimingsVGA[R].V_Visible - TTLV;
    uint32_t CenteringLinesTop = VGA_TTL_VerticalGap / 2;
    for (; Line < CenteringLinesTop; ++Line)
      DrawBlackLineWithMask4x1(/*InVSync=*/false);
  }
  uint32_t Offset = Line;
  // Note: We limit to min(TTL Visible, VGA V_Visble + V_FrontPorch) in case the
  // TTL input signal is slightly taller than VGA visible. This is useful for
  // some strange inputs that are 260 lines but we would still want to use VGA
  // 640x480.
  uint32_t DrawnLines =
      std::min(std::min((LineDoubling ? 2 : 1) * TimingsTTL.V_Visible,
                        TimingsVGA[R].V_Visible + TimingsVGA[R].V_FrontPorch),
               (LineDoubling ? 2 : 1) * DisplayBuffer::BuffY);
  uint32_t LineTTLE = Offset + DrawnLines;
  for (; Line < LineTTLE; ++Line)
    DrawLineVSyncHigh4x1((Line - Offset) / (LineDoubling ? 2 : 1));
  // 2. The rest is black, including the front porch
  static constexpr const uint32_t LineVGAE =
      TimingsVGA[R].V_Visible + TimingsVGA[R].V_FrontPorch;
  for (; Line < LineVGAE; ++Line)
    DrawBlackLineWithMask4x1(/*InVSync=*/false);

  for (uint32_t Line = 0; Line != TimingsVGA[R].V_Retrace; ++Line)
    DrawBlackLineWithMask4x1(/*InVSync=*/true);
}

template <VGAResolution R, bool LineDoubling>
void __not_in_flash_func(VGAWriter::drawFrame8x1)() {
  // 0. Non-visible Back porch
  for (uint32_t InvisLine = 0; InvisLine != TimingsVGA[R].V_BackPorch;
       ++InvisLine)
    DrawBlackLineWithMaskMDA8x1(VMaskMDA_8);

  // Visible
  // -------
  // 1. Vertical VGA-TTL visible gap, fill with black lines.
  uint32_t VGA_TTL_VerticalGap = TimingsVGA[R].V_Visible - TimingsTTL.V_Visible;
  uint32_t CenteringLinesTop = VGA_TTL_VerticalGap / 2;
  uint32_t TTLLine = 0;
  for (; TTLLine < CenteringLinesTop; ++TTLLine)
    DrawBlackLineWithMaskMDA8x1(VMaskMDA_8);

  // 2. Non-black TTL Visible.
  uint32_t LineTTLE =
      std::min(std::min((LineDoubling ? 2 : 1) * TimingsTTL.V_Visible,
                        TimingsVGA[R].V_Visible),
               (LineDoubling ? 2 : 1) * DisplayBuffer::BuffY);
  for (uint32_t BuffLine = 0; BuffLine < LineTTLE; ++BuffLine) {
    DrawLineVSyncHighMDA8x1(BuffLine / (LineDoubling ? 2 : 1));
    ++TTLLine;
  }
  // 3. Bottom part of visible to fill the gap with black and keep the image
  // centered.
  static constexpr const uint32_t LineVGAE = TimingsVGA[R].V_Visible;
  for (; TTLLine < LineVGAE; ++TTLLine)
    DrawBlackLineWithMaskMDA8x1(VMaskMDA_8);

  // 4. Non-visible front porch
  for (uint32_t Line = 0; Line != TimingsVGA[R].V_FrontPorch; ++Line)
    DrawBlackLineWithMaskMDA8x1(VMaskMDA_8);
  // 5. Retrace
  for (uint32_t Line = 0; Line != TimingsVGA[R].V_Retrace; ++Line)
    DrawBlackLineWithMaskMDA8x1(BlackMDA_8);
}

void VGAWriter::runForEver() {
  uint32_t Cnt = 0;
  while (true) {
    switch (TimingsTTL.Mode) {
    case TTL::CGA:
    case TTL::EGA:
      if (TTLReader::isHighRes(TimingsTTL))
        drawFrame4x1<VGA_640x400_70Hz, /*LineDoubing=*/false>();
      else {
        if (TimingsTTL.V_Visible - YB > 200)
          drawFrame4x1<VGA_640x480_60Hz, /*LineDoubing=*/true>();
        else
          drawFrame4x1<VGA_640x400_70Hz, /*LineDoubing=*/true>();
      }
      break;
    case TTL::MDA:
      // If this is real MDA with high horizontal resolution > 640 or vertical >
      // 240 use no line-doubling and 800x600.
      if (TimingsTTL.H_Visible - XB > 640 || TimingsTTL.V_Visible - YB > 240) {
        drawFrame8x1<VGA_800x600_56Hz, /*LineDoubling=*/false>();
      } else {
        // Use line-doubling
        if (TimingsTTL.V_Visible - YB > 200)
          // If this can vit in 640 horizontal but is > 200 vertically use
          // 640x480 with lien doubling.
          drawFrame8x1<VGA_640x480_60Hz, /*LineDoubing=*/true>();
        else
          // Use 640x400
          drawFrame8x1<VGA_640x400_70Hz, /*LineDoubing=*/true>();
      }
      break;
    default:
      DBG_PRINT(std::cout << "Unimplemented mode "
                          << modeToStr(TimingsTTL.Mode);)
      break;
    }
    ++Cnt;
    // Update PIO if needed.
    if (Cnt % 2 == 0) {
      tryChangePIOMode();
    }
    if (Cnt % 2 == 1)
      checkInputSignal();
  }
}
