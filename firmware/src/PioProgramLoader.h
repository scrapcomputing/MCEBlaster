//-*- C++ -*-
//
// Copyright (C) 2024 Scarp Computing
//

#ifndef __PIOPROGRAMLOADER_H__
#define __PIOPROGRAMLOADER_H__

#include "Debug.h"
#include "Utils.h"
#include "hardware/pio.h"
#include "pico/critical_section.h"
#include <functional>
#include <initializer_list>
#include <iostream>

class PioProgramLoader {
  struct KeyValuePair {
    std::pair<PIO, uint> Key;
    std::pair<const pio_program_t *, uint> Value;
  };
  Utils::FixedVector<KeyValuePair, 8> LoadedPrograms;
  critical_section LoadPIOProgramLock;

public:
  PioProgramLoader() { critical_section_init(&LoadPIOProgramLock); }

  /// \Returns the offset of the loaded program.
  uint loadPIOProgram(PIO Pio, uint SM, const pio_program_t *Program,
                      std::function<void(PIO, uint, uint)> Fn);
  void unloadAllPio(PIO Pio, std::initializer_list<uint> SMs);
};

#endif // __PIOPROGRAMLOADER_H__
