//-*- C++ -*-
//
// Copyright (C) 2025 Scrap Computing
//

#include "HorizMenu.h"
#include "TTLReader.h"

template <int MaxMenuItems>
void HorizMenu<MaxMenuItems>::addMenuItem(int MenuIdx, bool Enabled,
                                          const std::string &Prefix,
                                          const std::string &Txt) {
  if (MenuIdx >= NumMenuItems)
    NumMenuItems = MenuIdx + 1;
  MenuItems[MenuIdx] = MenuItem(Enabled, Prefix, Txt);
}

template <int MaxMenuItems>
void HorizMenu<MaxMenuItems>::display(int Selection, int DisplayTime) {
  std::string Line;
  for (int Idx = 0, E = NumMenuItems; Idx != E; ++Idx) {
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

template <int MaxMenuItems>
void HorizMenu<MaxMenuItems>::incrSelection(int &Selection) {
  do {
    Selection = (Selection + 1) % NumMenuItems;
  } while (!MenuItems[Selection].Enabled);
}

template <int MaxMenuItems>
void HorizMenu<MaxMenuItems>::decrSelection(int &Selection) {
  do {
    Selection = Selection == 0 ? NumMenuItems - 1 : Selection - 1;
  } while (!MenuItems[Selection].Enabled);
}

// Instantiation
template class HorizMenu<TTLReader::ManualTTLMenu_NumMenuItems>;
