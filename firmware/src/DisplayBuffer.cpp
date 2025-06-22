//-*- C++ -*-
//
// Copyright (C) 2024 Scarp Computing
//

#include "DisplayBuffer.h"
#include "TTLReader.h"
#include "xpm/splash.xpm"
#include <cstring>

DisplayBuffer::DisplayBuffer() : SplashXPM(splash) {
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
  CharMap.emplace(std::make_pair('B', Char_B));
  CharMap.emplace(std::make_pair('C', Char_C));
  CharMap.emplace(std::make_pair('D', Char_D));
  CharMap.emplace(std::make_pair('E', Char_E));
  CharMap.emplace(std::make_pair('F', Char_F));
  CharMap.emplace(std::make_pair('G', Char_G));
  CharMap.emplace(std::make_pair('H', Char_H));
  CharMap.emplace(std::make_pair('I', Char_I));
  CharMap.emplace(std::make_pair('J', Char_J));
  CharMap.emplace(std::make_pair('K', Char_K));
  CharMap.emplace(std::make_pair('L', Char_L));
  CharMap.emplace(std::make_pair('M', Char_M));
  CharMap.emplace(std::make_pair('N', Char_N));
  CharMap.emplace(std::make_pair('O', Char_O));
  CharMap.emplace(std::make_pair('P', Char_P));
  CharMap.emplace(std::make_pair('Q', Char_Q));
  CharMap.emplace(std::make_pair('R', Char_R));
  CharMap.emplace(std::make_pair('S', Char_S));
  CharMap.emplace(std::make_pair('T', Char_T));
  CharMap.emplace(std::make_pair('U', Char_U));
  CharMap.emplace(std::make_pair('V', Char_V));
  CharMap.emplace(std::make_pair('W', Char_W));
  CharMap.emplace(std::make_pair('X', Char_X));
  CharMap.emplace(std::make_pair('Y', Char_Y));
  CharMap.emplace(std::make_pair('Z', Char_Z));
  CharMap.emplace(std::make_pair('v', Char_v));
  CharMap.emplace(std::make_pair('x', Char_x));
  CharMap.emplace(std::make_pair('z', Char_z));
  CharMap.emplace(std::make_pair('.', Char_PERIOD));
  CharMap.emplace(std::make_pair(':', Char_COLON));
  CharMap.emplace(std::make_pair('?', Char_QUESTION));
  CharMap.emplace(std::make_pair('!', Char_EXCLAMATION));
  CharMap.emplace(std::make_pair('@', Char_AT));
  CharMap.emplace(std::make_pair(' ', Char_SPACE));
  CharMap.emplace(std::make_pair('-', Char_DASH));
  CharMap.emplace(std::make_pair('>', Char_GT));
  CharMap.emplace(std::make_pair('+', Char_PLUS));
  CharMap.emplace(std::make_pair('-', Char_MINUS));
  CharMap.emplace(std::make_pair('[', Char_SQBR_OPEN));
  CharMap.emplace(std::make_pair(']', Char_SQBR_CLOSE));

  // The DMA is used for copying txt to screen.
  DMAChannel = dma_claim_unused_channel(true);
}

void DisplayBuffer::clear() {
  memset(Buffer, 0, BuffX * BuffY);
}
void DisplayBuffer::clearTxtBuffer() {
  memset(TxtBuffer, 0, TxtBuffX * TxtBuffY);
}

void DisplayBuffer::noSignal() {
  clear();
  int Zoom = TTLReader::isHighRes(*TimingsTTL) ? 2 : 1;
  SplashXPM.show(*this, *TimingsTTL, 0, 0, Zoom);

  // Print the version at the bottom of the screen.
  std::string Version = std::string("v") +
                        std::to_string(REVISION_MAJOR) + "." +
                        std::to_string(REVISION_MINOR);
  displayTxt(Version, 0, /*Center=*/false);
  // Special case for MDA because we print 2 pixels per byte.
  int DisplayWidth = TimingsTTL->Mode == TTL::MDA ? TimingsTTL->H_Visible / 2
                                                  : TimingsTTL->H_Visible;
  for (int Line = 0; Line != BMapHeight; ++Line) {
    memcpy(&Buffer[TimingsTTL->V_Visible - 2 * TxtBuffY + Line]
                  [DisplayWidth - (Version.size() * 2) * BMapWidth],
           &TxtBuffer[Line][0], Version.size() * BMapWidth);
  }
}

void DisplayBuffer::setPixel(uint8_t Pixel, int X, int Y,
                             uint8_t Buff[][BuffX]) {
  int MaxBuffX;
  int MaxBuffY;
  if (Buff == Buffer) {
    MaxBuffX = BuffX;
    MaxBuffY = BuffY;
  } else if (Buff == TxtBuffer) {
    MaxBuffX = TxtBuffX;
    MaxBuffY = TxtBuffY;
  } else {
    Utils::unreachable("setPixel() Unimplemented Buff!");
  }
  // Avoid out of bounds accesses.
  if (X >= MaxBuffX || Y >= MaxBuffY || X < 0 || Y < 0) {
    DBG_PRINT(std::cout << __FUNCTION__ << " Out of bounds X=" << X
                        << " Y=" << Y << "\n";)
    return;
  }

  switch (TimingsTTL->Mode) {
  case TTL::CGA:
  case TTL::EGA:
    Buff[Y][X] = Pixel;
    break;
  case TTL::MDA: {
    bool IsFirst = X % 2 == 0;
    if (IsFirst) {
      Buff[Y][X / 2] &= 0xf0;
      Buff[Y][X / 2] |= Pixel & 0x0f;
    } else {
      Buff[Y][X / 2] &= 0x0f;
      Buff[Y][X / 2] |= Pixel & 0xf0;
    }
    break;
  }
  }
}

void DisplayBuffer::showBitmap(const uint8_t *BMap, int X, int Y,
                               uint8_t Buff[][BuffX], uint32_t FgColor,
                               uint32_t BgColor, int ZoomXLevel,
                               int ZoomYLevel) {
  for (int Line = 0; Line != BMapHeight; ++Line) {
    for (int Bit = 0; Bit != BMapWidth; ++Bit) {
      for (int ZoomX = 0; ZoomX != ZoomXLevel; ++ZoomX) {
        for (int ZoomY = 0; ZoomY != ZoomYLevel; ++ZoomY) {
          bool IsBitSet = BMap[Line] & (0b1 << Bit);
          uint8_t Pixel = IsBitSet ? FgColor : BgColor;
          setPixel(Pixel, X + ZoomXLevel * (BMapWidth - Bit) - ZoomX,
                   Y + ZoomYLevel * Line + ZoomY, Buff);
        }
      }
    }
  }
}

void DisplayBuffer::displayChar(char C, int X, int Y, uint8_t Buff[][BuffX],
                                uint32_t FgColor, uint32_t BgColor,
                                int ZoomXLevel, int ZoomYLevel) {
  const uint8_t *CharBMap = getCharSafe(C);
  showBitmap(CharBMap, X, Y, Buff, FgColor, BgColor, ZoomXLevel, ZoomYLevel);
}

void DisplayBuffer::displayPage(const std::string &PageTxt, bool Center) {
  clear();

  DBG_PRINT(std::cout << __FUNCTION__ << " PageTxt:\n" << PageTxt << "\n";)
  static constexpr const int LineSpacing = 0;
  int LeftBorder = 20;
  int TopBorder = 20;

  uint32_t FgColor = TimingsTTL->Mode == TTL::MDA ? White : BrightCyan;
  uint32_t BgColor = Black;

  if (Center) {
    // Find longest line.
    int MaxLineSz = 0;
    int NumLines = 0;
    for (int Idx = 0, LastNL = 0, E = PageTxt.size(); Idx != E; ++Idx) {
      if (PageTxt[Idx] == '\n') {
        MaxLineSz = std::max(MaxLineSz, Idx - LastNL);
        LastNL = Idx;
        ++NumLines;
      }
    }
    // Calculate the offset such that the text is centered.
    LeftBorder = TimingsTTL->H_Visible / 2 - MaxLineSz * BMapWidth / 2;
    TopBorder =
        TimingsTTL->V_Visible / 2 - NumLines * (BMapHeight + LineSpacing) / 2;
  }

  int X = LeftBorder;
  int Y = TopBorder;
  for (char C : PageTxt) {
    if (C == '\n') {
      Y += BMapHeight + LineSpacing;
      X = LeftBorder;
      continue;
    }
    displayChar(C, X, Y, Buffer, FgColor, BgColor);
    X += BMapWidth;
  }
  DBG_PRINT(std::cout << __FUNCTION__ << "----END----\n";)
}

void DisplayBuffer::displayTxt(const std::string &Line, int X, bool Center) {
  DBG_PRINT(std::cout << "displayTxt(" << Line << ")\n";)
  clearTxtBuffer();

  static constexpr const int ZoomXLevel = 1;
  static constexpr const int ZoomYLevel = 1;

  int ActualX =
      Center
          ? (TimingsTTL->H_Visible - Line.size() * BMapWidth * ZoomXLevel) / 2
          : X;
  int ActualY = 0;
  DBG_PRINT(std::cout << "displayTxt:>>" << Line << "<< (X=" << ActualX
                      << ",Y=" << ActualY << ")\n";)
  uint32_t FgColor = TimingsTTL->Mode == TTL::MDA ? White : BrightCyan;
  uint32_t BgColor = Black;
  for (char C : Line) {
    displayChar(C, ActualX, ActualY, TxtBuffer, FgColor, BgColor, ZoomXLevel,
                ZoomYLevel);
    ActualX += BMapWidth * ZoomXLevel;
  }
}

void DisplayBuffer::copyTxtBufferToScreen() {
  if (dma_channel_is_busy(DMAChannel))
    return;                     // Not sure it's needed
  dma_channel_config DMAConfig = dma_channel_get_default_config(DMAChannel);
  channel_config_set_transfer_data_size(&DMAConfig, DMA_SIZE_8);
  channel_config_set_read_increment(&DMAConfig, true);
  channel_config_set_write_increment(&DMAConfig, true);
  dma_channel_configure(DMAChannel, &DMAConfig,
                        /*Dst=*/&Buffer[getTxtLineYTop()][0],
                        /*Src=*/TxtBuffer,
                        /*Transfers=*/TxtBuffX * TxtBuffY,
                        true /*Start immediately*/);
}
