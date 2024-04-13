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
  // Use the last sector (4KB) of the 2MB flash.
  static constexpr const int BaseOffset = 2 * (1u << 20) - FLASH_SECTOR_SIZE;
  static constexpr const int BytesToFlash = FLASH_SECTOR_SIZE;
  std::vector<int> MagicNumber = {12131111, 41, 0, 667, 13121111};

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
