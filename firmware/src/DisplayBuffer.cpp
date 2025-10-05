//-*- C++ -*-
//
// Copyright (C) 2024 Scarp Computing
//

#include "DisplayBuffer.h"
#include "TTLReader.h"
#if defined(PICO_RP2040)
#include "xpm/splash_small.xpm" // To save memory!
#elif defined(PICO_RP2350)
#include "xpm/splash.xpm"
#endif
#include <cstring>

DisplayBuffer::DisplayBuffer() : SplashXPM(splash) {
  // The DMA is used for copying txt to screen.
  DMAChannel = dma_claim_unused_channel(true);
  DMAChannel2 = dma_claim_unused_channel(true);
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
  char Version[8];
  snprintf(Version, 8, "v%d.%d", REVISION_MAJOR, REVISION_MINOR);
  displayTxt(Version, 0, /*Center=*/false);
  // Special case for MDA because we print 2 pixels per byte.
  int DisplayWidth = TimingsTTL->Mode == TTL::MDA ? TimingsTTL->H_Visible / 2
                                                  : TimingsTTL->H_Visible;
  int VersionLen = strlen(Version);
  for (int Line = 0; Line != BMapHeight; ++Line) {
    memcpy(&Buffer[TimingsTTL->V_Visible - 2 * TxtBuffY + Line]
                  [DisplayWidth - (VersionLen * 2) * BMapWidth],
           &TxtBuffer[Line][0], VersionLen * BMapWidth);
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

void DisplayBuffer::displayPage(const Utils::StaticString<640> &PageTxt, bool Center) {
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

void DisplayBuffer::displayTxt(const char *Line, int X, bool Center) {
  DBG_PRINT(std::cout << "displayTxt(" << Line << ")\n";)
  clearTxtBuffer();

  static constexpr const int ZoomXLevel = 1;
  static constexpr const int ZoomYLevel = 1;

  int LineSz = strlen(Line);
  int ActualX =
      Center ? (TimingsTTL->H_Visible - LineSz * BMapWidth * ZoomXLevel) / 2
             : X;
  int ActualY = 0;
  DBG_PRINT(std::cout << "displayTxt:>>" << Line << "<< (X=" << ActualX
                      << ",Y=" << ActualY << ")\n";)
  uint32_t FgColor = TimingsTTL->Mode == TTL::MDA ? White : BrightCyan;
  uint32_t BgColor = Black;
  for (int Idx = 0; Idx != LineSz; ++Idx) {
    char C = Line[Idx];
    displayChar(C, ActualX, ActualY, TxtBuffer, FgColor, BgColor, ZoomXLevel,
                ZoomYLevel);
    ActualX += BMapWidth * ZoomXLevel;
  }
}

void DisplayBuffer::copyTxtBufferToScreen() {
  if (dma_channel_is_busy(DMAChannel))
    return;                     // Not sure it's needed
  dma_channel_config DMAConfig = dma_channel_get_default_config(DMAChannel);
  channel_config_set_transfer_data_size(&DMAConfig, DMA_SIZE_32);
  channel_config_set_read_increment(&DMAConfig, true);
  channel_config_set_write_increment(&DMAConfig, true);
  dma_channel_configure(DMAChannel, &DMAConfig,
                        /*Dst=*/&Buffer[getTxtLineYTop()][0],
                        /*Src=*/TxtBuffer,
                        /*Transfers=*/TxtBuffX * TxtBuffY / /*DMA_SIZE_32*/ 4,
                        true /*Start immediately*/);
}

void __not_in_flash_func(DisplayBuffer::fillBottomWithBlackAfter)(uint32_t Line) {
  if (Line >= BuffY)
    return;
  if (dma_channel_is_busy(DMAChannel2))
    return;                     // Not sure it's needed

  dma_channel_config DMAConfig = dma_channel_get_default_config(DMAChannel2);
  channel_config_set_transfer_data_size(&DMAConfig, DMA_SIZE_32);
  channel_config_set_read_increment(&DMAConfig, false);
  channel_config_set_write_increment(&DMAConfig, true);
  auto Transfers = (BuffY - Line) * BuffX / /*DMA_SIZE_32*/ 4;
  dma_channel_configure(DMAChannel2, &DMAConfig,
                        /*Dst=*/&Buffer[Line][0],
                        /*Src=*/&Black,
                        /*Transfers=*/Transfers,
                        true /*Start immediately*/);
}
