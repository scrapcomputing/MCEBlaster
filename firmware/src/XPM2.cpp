//-*- C++ -*-
//
// Copyright (C) 2024 Scarp Computing
//

#include "XPM2.h"
#include "DisplayBuffer.h"
#include "TTLReader.h"
#include "Timings.h"

void XPM2::show(DisplayBuffer &DBuff, const TTLDescr &TimingsTTL, int OffsetX,
                int OffsetY, int Zoom) {
  int DisplayWidth = TimingsTTL.H_Visible;
  int DisplayHeight = TimingsTTL.V_Visible;
  int ZoomX = Zoom;
  int ZoomY = Zoom;
  if (DBuff.TimingsTTL->Mode == TTL::MDA) {
    // Special case due to 2-pixels per byte
    DisplayWidth /= 2;
    ZoomX >>= 1;
  } else if ((DBuff.TimingsTTL->Mode == TTL::CGA ||
              DBuff.TimingsTTL->Mode == TTL::EGA) &&
             !TTLReader::isHighRes(*DBuff.TimingsTTL)) {
    // Special case due to pixel duplication on Y.
    ZoomX <<= 1;
  }
  int XRepeatedPixels = 1 << ZoomX;
  int YRepeatedPixels = 1 << ZoomY;
  int X =
      std::max(0, DisplayWidth / 2 + (OffsetX << ZoomX) - (Width / 2 << ZoomX));
  int Y = std::max(0, DisplayHeight / 2 + (OffsetY << ZoomY) -
                          (Height / 2 << ZoomY));
  auto &Buffer = DBuff.Buffer;
  for (int Line = 0, BuffLine = 0;
       Line < Height && Y + BuffLine + YRepeatedPixels < DisplayHeight;
       ++Line, BuffLine += YRepeatedPixels) {
    const char *LineStr = XPMArray[Line + NumColors + 1];
    for (int Col = 0, BuffCol = 0;
         Col < Width && X + BuffCol + XRepeatedPixels < DisplayWidth;
         ++Col, BuffCol += XRepeatedPixels) {
      uint8_t Pixel = CharToColor.at(LineStr[Col]);
      // Stamp the pixel multiple times to make a big zoomed-in pixel.
      for (int SubX = 0; SubX != XRepeatedPixels; ++SubX)
        for (int SubY = 0; SubY != YRepeatedPixels; ++SubY) {
          // TODO: Use DisplayBuffer's setters to set the pixel.
          Buffer[Y + BuffLine + SubY][X + BuffCol + SubX] = Pixel;
        }
    }
  }
}
