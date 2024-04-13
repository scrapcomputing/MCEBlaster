//-*- C++ -*-
//
// Copyright (C) 2024 Scarp Computing
//

#ifndef __XPM2_H__
#define __XPM2_H__

#include <unordered_map>

class DisplayBuffer;

class XPM2 {
  int Width;
  int Height;
  int NumColors;
  int CharsPerPixel;
  const char **XPMArray;
  std::unordered_map<char, uint8_t> CharToColor;
  void parseHeader() {
    // "Width Height NumColors CharsPerPixel"
    sscanf(XPMArray[0], "%d %d %d %d", &Width, &Height, &NumColors,
           &CharsPerPixel);
    // "Char c #Color"
    for (int ColorIdx = 0; ColorIdx != NumColors; ++ColorIdx) {
      char Char;
      uint32_t Color;
      sscanf(XPMArray[ColorIdx + 1], "%c c #%x", &Char, &Color);
      uint8_t R = (Color & 0xc00000) >> 18;
      uint8_t G = (Color & 0x00c000) >> 12;
      uint8_t B = (Color & 0x0000c0) >> 6;
      CharToColor[Char] = R | G | B;
    }
  }

  void parseBody(DisplayBuffer &DBuff, int OffsetX, int OffsetY, int Zoom) ;

public:
  XPM2(const char **XPMArray) : XPMArray(XPMArray) { parseHeader(); }
  uint32_t height() const { return Height; }
  uint32_t width() const { return Width; }
  void show(DisplayBuffer &DBuff, int OffsetX, int OffsetY, int Zoom = 2) {
    parseBody(DBuff, OffsetX, OffsetY, Zoom);
  }
};

#endif // __XPM2_H__
