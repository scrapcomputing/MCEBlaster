//-*- C++ -*-
//
// Copyright (C) 2024 Scrap Computing
//

#ifndef __FLASH_H__
#define __FLASH_H__

#include <config.h>
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <vector>

class FlashStorage {
  // Erase the last sector (4KB) of the 2MB flash, but write the last page (256
  // bytes).
  static constexpr const int BytesToErase = FLASH_SECTOR_SIZE;
  static constexpr const int BytesToWrite = FLASH_PAGE_SIZE;
  static constexpr const int EraseBaseOffset = 2 * (1u << 20) - BytesToErase;
  static constexpr const int WriteBaseOffset = 2 * (1u << 20) - BytesToWrite;
  std::vector<int> MagicNumber = {12131111, 43, 0, 667, 13121111};

  /// Points to the first usable int ptr, after the magic number and revision.
  const int *FlashArray = nullptr;

  std::vector<int> getDataVec(const std::vector<int> &Values);

public:
  FlashStorage();
  void write(const std::vector<int> &Values);
  /// \Reads a value at \p ValueIdx offset (after the magic numbers).
  int read(int ValueIdx) const {
    return FlashArray[ValueIdx];
  }
  std::pair<int, int> readRevision() const;
  std::vector<int> readMagicNumber() const;
  bool valid() const;
};

#endif // __FLASH_H__
