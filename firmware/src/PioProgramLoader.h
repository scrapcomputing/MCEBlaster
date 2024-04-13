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
  critical_section LoadPIOProgramLock; // Used in Utils.h

public:
  PioProgramLoader() { critical_section_init(&LoadPIOProgramLock); }

  /// \Returns the offset of the loaded program.
  uint loadPIOProgram(PIO Pio, uint SM, const pio_program_t *Program,
                      std::function<void(PIO, uint, uint)> Fn) {
    critical_section_enter_blocking(&LoadPIOProgramLock);
    // Key:   Pio, SM
    // Value: Program *, Offset
    DBG_PRINT(std::cout << "loadPIO: Disabling PIO " << Pio << "...\n";)
    pio_sm_set_enabled(Pio, SM, false);
    auto It = LoadedPrograms.find(std::make_pair(Pio, SM));
    if (It != LoadedPrograms.end()) {
      auto [CurrProgram, Offset] = It->second;
      DBG_PRINT(std::cout << "loadPIO: Removing PIO program " << CurrProgram << "...\n";)
      pio_remove_program(Pio, CurrProgram, Offset);
    }
    DBG_PRINT(std::cout << "loadPIO: Adding PIO program " << Program << "...\n";)
    uint Offset = pio_add_program(Pio, Program);
    DBG_PRINT(std::cout << "loadPIO: Offset=" << Offset << "\n";)
    Fn(Pio, SM, Offset);
    LoadedPrograms[std::make_pair(Pio, SM)] = {Program, Offset};
    pio_sm_set_enabled(Pio, SM, true);
    DBG_PRINT(std::cout << "loadPIO: Done!\n";)
    critical_section_exit(&LoadPIOProgramLock);
    return Offset;
  }
  void unloadAllPio(PIO Pio, const std::vector<uint> &SMs) {
    critical_section_enter_blocking(&LoadPIOProgramLock);
    DBG_PRINT(std::cout << "unloadAllPio:\n";)
    for (uint SM : SMs) {
      DBG_PRINT(std::cout << "Stopping SM " << SM << "\n";)
      pio_sm_set_enabled(Pio, SM, false);
      auto It = LoadedPrograms.find(std::make_pair(Pio, SM));
      if (It != LoadedPrograms.end()) {
        auto [CurrProgram, Offset] = It->second;
        DBG_PRINT(std::cout << "unloadAllPio: Removing PIO program " << CurrProgram
                            << "...\n";)
        pio_remove_program(Pio, CurrProgram, Offset);
        LoadedPrograms.erase(It);
      }
    }
    DBG_PRINT(std::cout << "unloadAllPio: Done removing\n";)
    critical_section_exit(&LoadPIOProgramLock);
  }
};

#endif // __PIOPROGRAMLOADER_H__
