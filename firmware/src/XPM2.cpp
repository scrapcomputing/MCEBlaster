//-*- C++ -*-
//
// Copyright (C) 2024 Scarp Computing
//

#include "XPM2.h"
#include "DisplayBuffer.h"

void XPM2::parseBody(DisplayBuffer &DBuff, int OffsetX, int OffsetY, int Zoom) {
  int DisplayWidth = Timing[DBuff.Mode][H_Visible];
  int DisplayHeight = Timing[DBuff.Mode][V_Visible];
  int ZoomX = Zoom;
  int ZoomY = Zoom;
  if (DBuff.Mode == MDA_720x350_50Hz) {
    // Special case due to pixel compression on X
    DisplayWidth /= 2;
    ZoomY <<= 1;
  } else if (DBuff.Mode == CGA_640x200_60Hz) {
    // Special case due to pixel duplication on Y.
    ZoomX <<= 1;
  }
  int X = DisplayWidth / 2 + (OffsetX << ZoomX) - (Width / 2 << ZoomX);
  int Y = DisplayHeight / 2 + (OffsetY << ZoomY) - (Height / 2 << ZoomY);
  auto &Buffer = DBuff.Buffer;
  for (int Line = 0, BuffLine = 0; Line != Height;
       ++Line, BuffLine += 1 << ZoomY) {
    const char *LineStr = XPMArray[Line + NumColors + 1];
    for (int Col = 0, BuffCol = 0; Col != Width; ++Col, BuffCol += 1 << ZoomX) {
      uint8_t Pixel = CharToColor.at(LineStr[Col]);
      for (int SubX = 0; SubX != 1 << ZoomX; ++SubX)
        for (int SubY = 0; SubY != 1 << ZoomY; ++SubY)
          // TODO: Use DisplayBuffer's setters to set the pixel.
          Buffer[Y + BuffLine + SubY][X + BuffCol + SubX] = Pixel;
    }
  }
}
