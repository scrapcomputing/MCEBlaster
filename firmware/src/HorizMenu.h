//-*- C++ -*-
//
// Copyright (C) 2025 Scrap Computing
//

#ifndef __HORIZMENU_H__
#define __HORIZMENU_H__

#include <array>

class TTLReader;

/// A single-line horizontal menu that uses TTLReader::displayTxt().
template <int MaxMenuItems> class HorizMenu {
  struct MenuItem {
    bool Enabled = false;
    const char *Prefix;
    const char *Item;
    MenuItem() = default;
    MenuItem(bool Enabled, const char *Prefix, const char *Item)
        : Enabled(Enabled), Prefix(Prefix), Item(Item) {}
  };

  TTLReader &TTLR;
  std::array<MenuItem, MaxMenuItems> MenuItems;
  int NumMenuItems = 0;

public:
  HorizMenu(TTLReader &TTLR) : TTLR(TTLR) {}
  void clearItems() { NumMenuItems = 0; }
  void addMenuItem(int MenuIdx, bool Enabled, const char *Prefix,
                   const char *Txt);
  void display(int Selection, int DisplayTime);
  /// Increment selection index, skipping disabled menu items.
  void incrSelection(int &Selection);
  /// Decrement selection index, skipping disabled menu items.
  void decrSelection(int &Selection);
};

#endif // __HORIZMENU_H__

