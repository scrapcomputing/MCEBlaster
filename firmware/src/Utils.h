//-*- C++ -*-
//
// Copyright (C) 2024 Scarp Computing
//

#ifndef __SRC_UTILS_H__
#define __SRC_UTILS_H__

#include "Debug.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <iterator>

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


  template <typename T, unsigned Sz>
  class FixedVector {
    std::array<T, Sz> Data;
    unsigned DataSz = 0;

  public:
    FixedVector() = default;
    void clear() { DataSz = 0; }
    unsigned size() const { return DataSz; }
    void push_back(const T &V) {
      assert(DataSz + 1 < Sz && "Out of bounds!");
      Data[DataSz++] = V;
    }
    T &operator[](unsigned Idx) {
      assert(Idx < Sz && "Out of bounds!");
      return Data[Idx];
    }
    const T& operator[](unsigned Idx) const {
      assert(Idx < Sz && "Out of bounds!");
      return Data[Idx];
    }
    class iterator {
      unsigned Idx = 0;
      FixedVector &Vec;

    public:
      using difference_type = std::ptrdiff_t;
      using value_type = T;
      using pointer = T *;
      using reference = T &;
      using iterator_category = std::input_iterator_tag;
      iterator(unsigned Idx, FixedVector &Vec) : Idx(Idx), Vec(Vec) {}
      iterator &operator++() {
        ++Idx;
        return *this;
      }
      T &operator*() {
        assert(Idx < Sz && "Already at end!");
        return Vec[Idx];
      }
      const T &operator*() const {
        assert(Idx < Sz && "Already at end!");
        return Vec[Idx];
      }
      bool operator==(const iterator &Other) const {
        assert(&Vec == &Other.Vec && "Comparing different vectors!");
        return Idx == Other.Idx;
      }
      bool operator!=(const iterator &Other) const { return !(*this == Other); }
    };
    void erase(iterator &It) {
      assert(It != end() && "Out of bounds!");
      assert(DataSz > 0 && "Already empty!");
      std::swap(*It, Data[DataSz - 1]);
      --DataSz;
    }
    iterator begin() { return iterator(0, *this); }
    iterator end() { return iterator(DataSz, *this); }
  };

  template <int StaticSz> class StaticString {
    int Sz = 0;
    char Buff[StaticSz];

  public:
    StaticString() { clear(); }
    StaticString &operator<<(const char *Str) {
      const char *C = Str;
      while (*C != '\0') {
        assert(Sz < StaticSz && "Out of bounds!");
        Buff[Sz++] = *C;
        ++C;
      }
      return *this;
    }
    StaticString &operator<<(int Num) {
      char TmpBuff[8];
      snprintf(TmpBuff, 8, "%d", Num);
      *this << TmpBuff;
      return *this;
    }
    operator const char *() const { return Buff; }
    const char *get() const { return Buff; }
    int size() const { return Sz; }
    using iterator = const char *;
    iterator begin() const { return Buff; }
    iterator end() const { return Buff + Sz; }
    void clear() {
      memset(Buff, '\0', StaticSz);
      Sz = 0;
    }
  };
};

#endif // __SRC_UTILS_H__
