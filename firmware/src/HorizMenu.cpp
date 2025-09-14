//-*- C++ -*-
//
// Copyright (C) 2025 Scrap Computing
//

#include "HorizMenu.h"
#include "TTLReader.h"

void HorizMenu::addMenuItem(int MenuIdx, bool Enabled,
                            const std::string &Prefix, const std::string &Txt) {
  if (MenuIdx >= (int)MenuItems.size())
    MenuItems.resize(MenuIdx + 1);
  MenuItems[MenuIdx] = MenuItem(Enabled, Prefix, Txt);
}

void HorizMenu::display(int Selection, int DisplayTime) {
  std::string Line;
  for (int Idx = 0, E = MenuItems.size(); Idx != E; ++Idx) {
    const MenuItem &MItem = MenuItems[Idx];
    if (!MItem.Enabled) {
      continue;
    }
    const char *SelectL = Idx == Selection ? "[" : " ";
    const char *SelectR = Idx == Selection ? "]" : " ";
    Line += MItem.Prefix + SelectL + MItem.Item + SelectR;
  }
  TTLR.displayTxt(Line, DisplayTime);
}

void HorizMenu::incrSelection(int &Selection) {
  do {
    Selection = (Selection + 1) % MenuItems.size();
  } while (!MenuItems[Selection].Enabled);
}

void HorizMenu::decrSelection(int &Selection) {
  do {
    Selection = Selection == 0 ? (int)MenuItems.size() - 1 : Selection - 1;
  } while (!MenuItems[Selection].Enabled);
}
