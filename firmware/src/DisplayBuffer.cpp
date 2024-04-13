//-*- C++ -*-
//
// Copyright (C) 2024 Scarp Computing
//

#include "DisplayBuffer.h"
#include "xpm/splash.xpm"
#include <cstring>

DisplayBuffer::DisplayBuffer() {
  Modes[MDA_720x350_50Hz] = {/*ns=*/63,
    /*PIO=*/10}; // 63ns->15.75(=16) instrs PIO=10
  Modes[CGA_640x200_60Hz] = {/*ns=*/71, /*PIO=*/28}; // 35.4 instrs / pixel
  Modes[EGA_640x200_60Hz] = {/*ns=*/71, /*PIO=*/28};
  Modes[EGA_640x350_60Hz] = {/*ns=*/51,
    /*PIO=*/7}; // 51ns->12.75(=13) instrs PIO=7

  CharMap.emplace(std::make_pair('0', Char_0));
  CharMap.emplace(std::make_pair('1', Char_1));
  CharMap.emplace(std::make_pair('2', Char_2));
  CharMap.emplace(std::make_pair('3', Char_3));
  CharMap.emplace(std::make_pair('4', Char_4));
  CharMap.emplace(std::make_pair('5', Char_5));
  CharMap.emplace(std::make_pair('6', Char_6));
  CharMap.emplace(std::make_pair('7', Char_7));
  CharMap.emplace(std::make_pair('8', Char_8));
  CharMap.emplace(std::make_pair('9', Char_9));
  CharMap.emplace(std::make_pair('A', Char_A));
  CharMap.emplace(std::make_pair('C', Char_C));
  CharMap.emplace(std::make_pair('D', Char_D));
  CharMap.emplace(std::make_pair('E', Char_E));
  CharMap.emplace(std::make_pair('F', Char_F));
  CharMap.emplace(std::make_pair('G', Char_G));
  CharMap.emplace(std::make_pair('H', Char_H));
  CharMap.emplace(std::make_pair('I', Char_I));
  CharMap.emplace(std::make_pair('L', Char_L));
  CharMap.emplace(std::make_pair('J', Char_J));
  CharMap.emplace(std::make_pair('K', Char_K));
  CharMap.emplace(std::make_pair('M', Char_M));
  CharMap.emplace(std::make_pair('N', Char_N));
  CharMap.emplace(std::make_pair('O', Char_O));
  CharMap.emplace(std::make_pair('P', Char_P));
  CharMap.emplace(std::make_pair('R', Char_R));
  CharMap.emplace(std::make_pair('S', Char_S));
  CharMap.emplace(std::make_pair('T', Char_T));
  CharMap.emplace(std::make_pair('U', Char_U));
  CharMap.emplace(std::make_pair('V', Char_V));
  CharMap.emplace(std::make_pair('W', Char_W));
  CharMap.emplace(std::make_pair('X', Char_X));
  CharMap.emplace(std::make_pair('Y', Char_Y));
  CharMap.emplace(std::make_pair('x', Char_x));
  CharMap.emplace(std::make_pair('z', Char_z));
  CharMap.emplace(std::make_pair('.', Char_PERIOD));
  CharMap.emplace(std::make_pair(':', Char_COLON));
  CharMap.emplace(std::make_pair('?', Char_QUESTION));
  CharMap.emplace(std::make_pair('!', Char_EXCLAMATION));
  CharMap.emplace(std::make_pair('@', Char_AT));
  CharMap.emplace(std::make_pair(' ', Char_SPACE));
}

void DisplayBuffer::clear() {
  memset(Buffer, 0, BuffX * BuffY);
}

void DisplayBuffer::noSignal() {
  clear();
  XPM2 SplashXPM(splash);
  int Zoom = 1;
  switch(Mode) {
  case CGA_640x200_60Hz:
    Zoom = 1;
    break;
  case EGA_640x350_60Hz:
    Zoom = 2;
    break;
  case MDA_720x350_50Hz:
    Zoom = 1;
    break;
  }
  SplashXPM.show(*this, 0, 0, Zoom);
}

void DisplayBuffer::setPixel(uint8_t Pixel, int X, int Y) {
  switch (Mode) {
  case CGA_640x200_60Hz:
  case EGA_640x350_60Hz:
    Buffer[Y][X] = Pixel;
    break;
  case MDA_720x350_50Hz: {
    bool IsFirst = X % 2 == 0;
    if (IsFirst) {
      Buffer[Y][X / 2] &= 0xf0;
      Buffer[Y][X / 2] |= Pixel & 0x0f;
    } else {
      Buffer[Y][X / 2] &= 0x0f;
      Buffer[Y][X / 2] |= Pixel & 0xf0;
    }
    break;
  }
  }
}

void DisplayBuffer::showBitmap(const uint8_t *BMap, int X, int Y,
                               int ZoomXLevel, int ZoomYLevel) {
  uint32_t ONPixel = Mode == MDA_720x350_50Hz ? White : BrightCyan;
  uint32_t OFFPixel = Mode == Black;
  for (int Line = 0; Line != BMapHeight; ++Line) {
    for (int Bit = 0; Bit != BMapWidth; ++Bit) {
      for (int ZoomX = 0; ZoomX != ZoomXLevel; ++ZoomX) {
        for (int ZoomY = 0; ZoomY != ZoomYLevel; ++ZoomY) {
          bool IsBitSet = BMap[Line] & (0b1 << Bit);
          uint8_t Pixel = IsBitSet ? ONPixel : OFFPixel;
          setPixel(Pixel, X + ZoomXLevel * (BMapWidth - Bit) - ZoomX,
                   Y + ZoomYLevel * Line + ZoomY);
        }
      }
    }
  }
}

void DisplayBuffer::displayTxt(const std::string &TxtOrig, int X, int Y, int Zoom,
                               bool Center) {
  DBG_PRINT(std::cout << "displayTxt(" << TxtOrig << ")\n";)

  // Add NL, makes it easier to process.
  std::string Txt = TxtOrig + "\n";
  // Find number of lines and max line size.
  int MaxLineSz = 0;
  int LastNLIdx = -1;
  std::vector<std::string> Lines;
  for (int i = 0, e = Txt.size(); i != e; ++i) {
    char C = Txt[i];
    if (C == '\n') {
      int LineSz = i - (LastNLIdx + 1);
      MaxLineSz = std::max(MaxLineSz, LineSz);
      Lines.push_back(Txt.substr(LastNLIdx + 1, LineSz));
      LastNLIdx = i;
    }
  }


  bool YDoublingMode = Mode == CGA_640x200_60Hz || Mode == EGA_640x200_60Hz;
  int ZoomXLevel = Zoom;
  int ZoomYLevel = Zoom;
  if (Zoom > 1)
    ZoomYLevel = YDoublingMode ? Zoom >> 1 : Zoom;
  else
    ZoomXLevel = YDoublingMode ? Zoom << 1 : Zoom;

  int LineIdx = 0;
  for (const std::string Line : Lines) {
    ++LineIdx;
    std::cout << "Line:>>" << Line << "<<\n";
    int ActualX = Center ? Timing[Mode][H_Visible] / 2 -
                               Line.size() * BMapWidth * ZoomXLevel / 2
                         : X;
    // Note: This is "almost" centered.
    int ActualY = Center
                      ? Timing[Mode][V_Visible] / 2 -
                            BMapHeight * (Lines.size() - LineIdx) * ZoomYLevel
                      : Y;
    for (char C : Line) {
      const uint8_t *CharBMap = getCharSafe(C);
      showBitmap(CharBMap, ActualX, ActualY, ZoomXLevel, ZoomYLevel);
      ActualX += BMapWidth * Zoom;
    }
  }
}
