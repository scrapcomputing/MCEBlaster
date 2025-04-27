//-*- C++ -*-
//
// Copyright (C) 2024 Scarp Computing
//

#ifndef __PIOPROGRAMLOADER_H__
#define __PIOPROGRAMLOADER_H__

#include "Debug.h"
#include "hardware/pio.h"
#include "pico/critical_section.h"
#include <functional>
#include <iostream>
#include <map>

class PioProgramLoader {
  std::map<std::pair<PIO, uint>, std::pair<const pio_program_t *, uint>>
      LoadedPrograms;
  critical_section LoadPIOProgramLock;

public:
  PioProgramLoader() { critical_section_init(&LoadPIOProgramLock); }

  /// \Returns the offset of the loaded program.
  uint loadPIOProgram(PIO Pio, uint SM, const pio_program_t *Program,
                      std::function<void(PIO, uint, uint)> Fn);
  void unloadAllPio(PIO Pio, const std::vector<uint> &SMs);
};

#endif // __PIOPROGRAMLOADER_H__
