//-*- C++ -*-
//
// Copyright (C) 2024 Scrap Computing
//

#include "Flash.h"
#include "Debug.h"
#include <cstring>
#include <iostream>
#include <pico/multicore.h>

FlashStorage::DataTy::DataTy() {
  std::fill(Elements.begin(), Elements.end(), 0);
  // 1. Magic number.
  int Idx = MagicNumberIdx;
  for (int M : MagicNumber)
    Elements[Idx++] = M;
  // 2. Revision.
  Elements[RevisionMajorIdx] = REVISION_MAJOR;
  Elements[RevisionMinorIdx] = REVISION_MINOR;
}

void FlashStorage::DataTy::dump() const {
  for (uint32_t Idx = 0, E = ActualDataStartIdx; Idx < E; ++Idx)
    std::cout << Idx << " : " << Elements[Idx] << "\n";
  std::cout << "-- Actual Data --\n";
  for (uint32_t Idx = ActualDataStartIdx, E = Elements.size(); Idx < E; ++Idx)
    std::cout << Idx << " : " << Elements[Idx] << "\n";
}

FlashStorage::FlashStorage() {
  FlashArray = (const int *)(XIP_BASE + WriteBaseOffset);
}

void FlashStorage::write(const DataTy &DataVec) {
  DBG_PRINT(std::cout << "WriteBaseOffset=" << WriteBaseOffset
                      << " BytesToWrite=" << BytesToWrite << "\n";)
  DBG_PRINT(std::cout << "Written values:\n";)
  DBG_PRINT(DataVec.dump();)
  DBG_PRINT(std::cout << "Before save and disable interrupts()\n";)
  // When writing to flash we need to stop the other core from running code.
  // Please note that the other core shoudl: multicore_lockout_victim_init()
  multicore_lockout_start_blocking();
  // We also need to disable interrupts.
  uint32_t SvInterrupts = save_and_disable_interrupts();
  flash_range_erase(EraseBaseOffset, BytesToErase);
  flash_range_program(WriteBaseOffset, (const uint8_t *)DataVec.raw_get(),
                      DataVec.size_in_bytes());
  // Restore interrupts.
  restore_interrupts(SvInterrupts);
  // Resume execution on other core.
  multicore_lockout_end_blocking();
  DBG_PRINT(std::cout << "After interrupts\n";)
}

FlashStorage::MagicNumberTy FlashStorage::readMagicNumber() const {
  MagicNumberTy ReadMN;
  for (unsigned Idx = 0, E = ReadMN.size(); Idx != E; ++Idx)
    ReadMN[Idx] = FlashArray[MagicNumberIdx + Idx];

  DBG_PRINT(std::cout << "Read magic number: ";)
  DBG_PRINT(for (int V : ReadMN) std::cout << V << " ";)
  DBG_PRINT(std::cout << "\n";)
  return ReadMN;
}

std::pair<int, int> FlashStorage::readRevision() const {
  return {FlashArray[RevisionMajorIdx], FlashArray[RevisionMinorIdx]};
}

bool FlashStorage::valid() const {
  auto [Major, Minor] = readRevision();
  return readMagicNumber() == MagicNumber && Major == REVISION_MAJOR;
}
