//-*- C++ -*-
//
// Copyright (C) 2025 Scrap Computing
//

#ifndef __HORIZMENU_H__
#define __HORIZMENU_H__

#include <vector>
#include <string>

class TTLReader;

/// A single-line horizontal menu that uses TTLReader::displayTxt().
class HorizMenu {
  struct MenuItem {
    bool Enabled = false;
    std::string Prefix;
    std::string Item;
    MenuItem() = default;
    MenuItem(bool Enabled, const std::string &Prefix, const std::string &Item)
        : Enabled(Enabled), Prefix(Prefix), Item(Item) {}
  };

  TTLReader &TTLR;
  std::vector<MenuItem> MenuItems;

public:
  HorizMenu(TTLReader &TTLR) : TTLR(TTLR) {}
  void clearItems() { MenuItems.clear(); }
  void addMenuItem(int MenuIdx, bool Enabled, const std::string &Prefix,
                   const std::string &Txt);
  void display(int Selection, int DisplayTime);
  /// Increment selection index, skipping disabled menu items.
  void incrSelection(int &Selection);
  /// Decrement selection index, skipping disabled menu items.
  void decrSelection(int &Selection);
};

#endif // __HORIZMENU_H__

