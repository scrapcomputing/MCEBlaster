//-*- C++ -*-
//
// Copyright (C) 2024 Scarp Computing
//

#ifndef __SRC_UTILS_H__
#define __SRC_UTILS_H__

#include "Debug.h"
#include <array>
#include <cstdint>
#include <iostream>

#define DUMP_METHOD __attribute__((noinline)) __attribute__((__used__))

struct Utils {
  static void printBin(uint32_t Num, uint32_t Bits) {
    for (int Idx = Bits - 1; Idx >= 0; --Idx) {
      bool Bit = Num & (1u << Idx);
      std::cout << (int)Bit;
    }
  }

  static void sleep_ns(uint32_t ns) {
    for (int i = 0, e = ns / 8; i != e; ++i)
      asm volatile("nop\n"); /* 8ns each 1 cycle @125MHz */
  }

  /// sleep_ms() on one core seems to be interfering with the other core
  static void sleep_ms(uint32_t ms) {
    // NOTE: This works for 270 MHz
    sleep_ns(ms * 1000 * 350);
  }

  static inline void sleep_nops(int nops) {
    for (int i = 0; i != nops; ++i)
      asm volatile("nop:"); /* 8ns each 1 cycle @125MHz */
  }

#define sleep_80ns()                                                           \
  asm volatile("nop\n");                                                        \
  asm volatile("nop\n");                                                        \
  asm volatile("nop\n");                                                        \
  asm volatile("nop\n");                                                        \
  asm volatile("nop\n");                                                        \
  asm volatile("nop\n");                                                        \
  asm volatile("nop\n");                                                        \
  asm volatile("nop\n");                                                        \
  asm volatile("nop\n");                                                        \
  asm volatile("nop\n");

#define sleep_800ns()                                                          \
  sleep_80ns();                                                                \
  sleep_80ns();                                                                \
  sleep_80ns();                                                                \
  sleep_80ns();                                                                \
  sleep_80ns();                                                                \
  sleep_80ns();                                                                \
  sleep_80ns();                                                                \
  sleep_80ns();                                                                \
  sleep_80ns();                                                                \
  sleep_80ns();

  /// Holds the last several entries.
  /// Can return the mean.
  template <typename T, unsigned Sz, T InitVal> class Buffer {
    std::array<T, Sz> Vec;

  public:
    Buffer() {
      for (unsigned Cnt = 0; Cnt != Sz; ++Cnt)
        Vec[Cnt] = InitVal;
    }
    void append(T Val) {
      // Shift left
      for (unsigned Cnt = 0; Cnt + 1 < Sz; ++Cnt)
        Vec[Cnt] = Vec[Cnt + 1];
      Vec[Sz - 1] = Val;
    }
    T getMean() const {
      int Sum = 0;
      int CurrSz = 0;
      for (int Val : Vec) {
        Sum += Val;
        CurrSz += 1;
      }
      return (T)(Sum / CurrSz);
    }
    T operator[](unsigned Idx) const { return Vec[Idx]; }
    T back() const { return Vec.back(); }
    void clear() { std::fill(Vec.begin(), Vec.end(), InitVal); }
  };

  [[noreturn]] static void unreachable(const std::string &Msg) {
    DBG_PRINT(std::cerr << Msg << "\n";)
    exit(1);
  }
};

#endif // __SRC_UTILS_H__
