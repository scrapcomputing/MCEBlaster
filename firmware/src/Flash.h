//-*- C++ -*-
//
// Copyright (C) 2024 Scrap Computing
//

#ifndef __FLASH_H__
#define __FLASH_H__

#include "Debug.h"
#include <array>
#include <config.h>
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <iostream>

class FlashStorage {
  // Erase the last sector (4KB) of the 2MB flash, but write the last page (256
  // bytes).
  static constexpr const int BytesToErase = FLASH_SECTOR_SIZE;
  static constexpr const int BytesToWrite = FLASH_PAGE_SIZE;

  static constexpr const int EraseBaseOffset = 2 * (1u << 20) - BytesToErase;
  static constexpr const int WriteBaseOffset = 2 * (1u << 20) - BytesToWrite;

  using MagicNumberTy = std::array<int, 5>;
  static constexpr MagicNumberTy MagicNumber = {12131111, 45, 0, 667, 13121111};

  static constexpr const int MagicNumberIdx = 0;
  static constexpr const int RevisionMajorIdx = MagicNumber.size();
  static constexpr const int RevisionMinorIdx = RevisionMajorIdx + 1;

  static constexpr const int ActualDataStartIdx = RevisionMinorIdx + 1;

  /// Points to the beginning of the writeable area.
  const int *FlashArray = nullptr;

public:
  /// Hides the boilerplate elements (version and magic number) written to
  /// flash.
  class DataTy {
    static constexpr const size_t NumElms = BytesToWrite / sizeof(int);
    std::array<int, NumElms> Elements;

  public:
    DataTy();
    using ValTy = int;
    ValTy &operator[](int Idx) {
      int ActualIdx = ActualDataStartIdx + Idx;
      assert(ActualIdx < (int)Elements.size() && "Out of bounds!");
      return Elements[ActualIdx];
    }
    const ValTy &operator[](int Idx) const {
      int ActualIdx = ActualDataStartIdx + Idx;
      assert(ActualIdx < (int)Elements.size() && "Out of bounds!");
      return Elements.at(ActualIdx);
    }
    /// \Returns the raw data array (including the boilerplate).
    const ValTy *raw_get() const { return Elements.data(); }
  public:
    /// \Returns the raw data size (including the boilerplate).
    static constexpr size_t size_in_bytes() { return NumElms * sizeof(ValTy); }
    void dump() const;
  };
  FlashStorage();
  void write(const DataTy &DataVec);
  /// \Reads a value at \p ValueIdx offset (after the boilerplate).
  int read(int ValueIdx) const {
    auto Val = FlashArray[ActualDataStartIdx + ValueIdx];
    DBG_PRINT(std::cout << "Flash.read(" << ValueIdx << ") : " << Val << "\n";)
    return Val;
  }
  /// \Returns the memory location of the actual data (not boilerplate) in flash
  const int *getData() const { return &FlashArray[ActualDataStartIdx]; }
  std::pair<int, int> readRevision() const;
  MagicNumberTy readMagicNumber() const;
  bool valid() const;
};

#endif // __FLASH_H__
