//-*- C++ -*-
//
// Copyright (C) 2024 Scrap Computing
//

#include "Common.h"
#include "Debug.h"
#include "Flash.h"
#include "Pico.h"
#include "TTLReader.h"
#include "VGAWriter.h"
#include <pico/multicore.h>

FlashStorage *Flash = nullptr;
Pico *Pi = nullptr;
PioProgramLoader *PPL = nullptr;

static void core1_main() {
  TTLReader TTLR(*PPL, *Pi, *Flash);
  TTLR.runForEver();
}

static void core0_main_loop(Pico &Pico, FlashStorage &Flash) {
  VGAWriter VGAW(Pico, *PPL);
  VGAW.runForEver();
}

// Hack for linking error: undefined reference to `__dso_handle'
static void *__dso_handle = 0;

critical_section UnusedPIOLock;      // Used in VGAWriter.cpp

int main() {
  (void)__dso_handle;
  Pico Pico;
  Pi = &Pico;
  critical_section_init(&UnusedPIOLock);

  DBG_PRINT(sleep_ms(1000);)
  DBG_PRINT(std::cout << PROJECT_NAME << " rev." << REVISION_MAJOR << "."
                      << REVISION_MINOR << "\n";)
  Pico.initGPIO(PICO_DEFAULT_LED_PIN, GPIO_OUT, Pico::Pull::Down, "LED");
  Pico.initGPIO(AUTO_ADJUST_GPIO, GPIO_IN, Pico::Pull::Up, "AutoAdjust");
  // Note: pull up seems to help with level shifting
  Pico.initGPIO(PinRange(EGA_RGB_GPIO, EGA_RGB_GPIO + 6), GPIO_IN,
                Pico::Pull::Up, "EGA");
  Pico.initGPIO(PinRange(TTL_VSYNC_GPIO), GPIO_IN, Pico::Pull::Up, "TTL_VSync");
  Pico.initGPIO(PinRange(TTL_HSYNC_GPIO), GPIO_IN, Pico::Pull::Up, "TTL_HSync");
  Pico.initGPIO(PinRange(CGA_ACTUAL_RGB_GPIO, CGA_ACTUAL_RGB_GPIO + 6), GPIO_IN,
                Pico::Pull::Up, "CGA");
  Pico.initGPIO(PinRange(VGA_RGB_GPIO, VGA_RGB_GPIO + 6), GPIO_OUT,
                Pico::Pull::Down, "VGA");
  Pico.initGPIO(PinRange(MDA_VI_GPIO, MDA_VI_GPIO + 2), GPIO_IN, Pico::Pull::Up,
                "MDA");

  // Read Presets from flash.
  FlashStorage FlashMain;
  Flash = &FlashMain;
  PioProgramLoader PioLoader;
  PPL = &PioLoader;

  multicore_launch_core1(core1_main);
  core0_main_loop(Pico, FlashMain);
  return 0;
}
