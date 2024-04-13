//-*- C++ -*-
//
// Copyright (C) 2024 Scrap Computing
//

#include "Flash.h"
#include "Debug.h"
#include <iostream>
#include <pico/multicore.h>

std::vector<int> FlashStorage::getDataVec(const std::vector<int> &Values) {
  std::vector<int> TmpDataVec;
  int NumEntries = BytesToFlash / sizeof(TmpDataVec[0]);
  TmpDataVec.reserve(NumEntries);
  TmpDataVec.reserve(MagicNumber.size() + 2 + Values.size());
  // 1. Magic number.
  TmpDataVec.insert(TmpDataVec.end(), MagicNumber.begin(), MagicNumber.end());
  // 2. Revision.
  TmpDataVec.push_back(REVISION_MAJOR);
  TmpDataVec.push_back(REVISION_MINOR);
  // 3. Actual values.
  TmpDataVec.insert(TmpDataVec.end(), Values.begin(), Values.end());
  // // 5. Fill the rest with zeros.
  // for (int Cnt = TmpDataVec.size(); Cnt != NumEntries; ++Cnt)
  //   TmpDataVec.push_back(0);
  return TmpDataVec;
}

FlashStorage::FlashStorage() {
  FlashArray =
      (const int *)(XIP_BASE + BaseOffset + /*Revision:*/ 2 * sizeof(int) +
                    /*MagicNumber:*/ MagicNumber.size() *
                        sizeof(MagicNumber[0]));
}

void FlashStorage::write(const std::vector<int> &Values) {
  DBG_PRINT(std::cout << "BaseOffset=" << BaseOffset
                      << " BytesToFlash=" << BytesToFlash << "\n";)
  DBG_PRINT(std::cout << "DataVec:\n";)
  auto DataVec = getDataVec(Values);
  for (int Val : DataVec)
    DBG_PRINT(std::cout << Val << "\n";)

  DBG_PRINT(std::cout << "Before save and disable interrupts()\n";)
  // When writing to flash we need to stop the other core from running code.
  // Please note that the other core shoudl: multicore_lockout_victim_init()
  multicore_lockout_start_blocking();
  // We also need to disable interrupts.
  uint32_t SvInterrupts = save_and_disable_interrupts();
  flash_range_erase(BaseOffset, BytesToFlash);
  flash_range_program(BaseOffset, (const uint8_t *)DataVec.data(),
                      BytesToFlash);
  // Restore interrupts.
  restore_interrupts(SvInterrupts);
  // Resume execution on other core.
  multicore_lockout_end_blocking();
  DBG_PRINT(std::cout << "After interrupts\n";)
}

std::vector<int> FlashStorage::readMagicNumber() const {
  std::vector<int> MagicNum;
  int MagicNumberSz = (int)MagicNumber.size();
  for (int Idx = 0; Idx != MagicNumberSz; ++Idx)
    MagicNum.push_back(FlashArray[Idx - MagicNumberSz - 2]);

  DBG_PRINT(std::cout << "Read magic number: ";)
  for (int V : MagicNum)
    DBG_PRINT(std::cout << V << " ";)
  DBG_PRINT(std::cout << "\n";)
  return MagicNum;
}

std::pair<int, int> FlashStorage::readRevision() const {
  int MagicNumberSz = (int)MagicNumber.size();
  return {FlashArray[-2], FlashArray[-1]};
}

bool FlashStorage::valid() const { return readMagicNumber() == MagicNumber; }
