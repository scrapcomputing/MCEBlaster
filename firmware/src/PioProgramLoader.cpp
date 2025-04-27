//-*- C++ -*-
//
// Copyright (C) 2025 Scarp Computing
//

#include "PioProgramLoader.h"

#ifdef DBGPRINT
static const char *getPioStr(PIO Pio) {
  return Pio == pio0 ? "Pio0" : Pio == pio1 ? "Pio1" : "UnknownPIO";
}
#endif

uint PioProgramLoader::loadPIOProgram(PIO Pio, uint SM,
                                      const pio_program_t *Program,
                                      std::function<void(PIO, uint, uint)> Fn) {
  DBG_PRINT(std::cout << "loadPIO: Before critical section " << getPioStr(Pio)
                      << " SM=" << SM << "\n";)
  critical_section_enter_blocking(&LoadPIOProgramLock);
  // Key:   Pio, SM
  // Value: Program *, Offset
  DBG_PRINT(std::cout << "loadPIO: Disabling " << getPioStr(Pio) << " SM=" << SM
                      << "...\n";)
  pio_sm_set_enabled(Pio, SM, false);

  // Empty the Fifo
  while (!pio_sm_is_rx_fifo_empty(Pio, SM))
    pio_sm_get_blocking(Pio, SM);

  auto It = LoadedPrograms.find(std::make_pair(Pio, SM));
  if (It != LoadedPrograms.end()) {
    auto [CurrProgram, Offset] = It->second;
    DBG_PRINT(std::cout << "loadPIO: Removing " << getPioStr(Pio) << " program "
                        << CurrProgram << "...\n";)
    pio_remove_program(Pio, CurrProgram, Offset);
  }
  DBG_PRINT(std::cout << "loadPIO: Adding " << getPioStr(Pio) << " program "
                      << Program << "...\n";)
  uint Offset = pio_add_program(Pio, Program);
  DBG_PRINT(std::cout << "loadPIO: Offset=" << Offset << "\n";)
  Fn(Pio, SM, Offset);
  LoadedPrograms[std::make_pair(Pio, SM)] = {Program, Offset};
  pio_sm_set_enabled(Pio, SM, true);
            DBG_PRINT(std::cout << "loadPIO: Enabled " << getPioStr(Pio)
                      << " SM=" << SM << "\n";)
  critical_section_exit(&LoadPIOProgramLock);
  DBG_PRINT(std::cout << "loadPIO: after critical section\n";)
  return Offset;
}

void PioProgramLoader::unloadAllPio(PIO Pio, const std::vector<uint> &SMs) {
  critical_section_enter_blocking(&LoadPIOProgramLock);
  DBG_PRINT(std::cout << "unloadAllPio:\n";)
  for (uint SM : SMs) {
    DBG_PRINT(std::cout << "Stopping SM " << SM << "\n";)
    pio_sm_set_enabled(Pio, SM, false);

    // Empty the Fifo
    while (!pio_sm_is_rx_fifo_empty(Pio, SM))
      pio_sm_get_blocking(Pio, SM);

    auto It = LoadedPrograms.find(std::make_pair(Pio, SM));
    if (It != LoadedPrograms.end()) {
      auto [CurrProgram, Offset] = It->second;
      DBG_PRINT(std::cout << "unloadAllPio: Removing PIO program "
                          << CurrProgram << "...\n";)
      pio_remove_program(Pio, CurrProgram, Offset);
      LoadedPrograms.erase(It);
    }
  }
  DBG_PRINT(std::cout << "unloadAllPio: Done removing\n";)
  critical_section_exit(&LoadPIOProgramLock);
}
