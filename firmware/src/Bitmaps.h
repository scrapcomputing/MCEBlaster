//-*- C++ -*-
//
// Copyright (C) 2024 Scarp Computing
//

// clang-format off

#ifndef __BITMAPS_H__
#define __BITMAPS_H__

#include <cstdint>

static constexpr const int BMapWidth = 8;
static constexpr const int BMapHeight = 8;

static constexpr const uint8_t Char_0[] {
  0b00000000,
  0b00111100,
  0b01100110,
  0b01100110,
  0b01100110,
  0b01100110,
  0b00111100,
  0b00000000,
};

static constexpr const uint8_t Char_1[] {
  0b00000000,
  0b00011000,
  0b00111000,
  0b00011000,
  0b00011000,
  0b00011000,
  0b00011000,
  0b00000000,
};

static constexpr const uint8_t Char_2[] {
  0b00000000,
  0b00111100,
  0b01100110,
  0b00001100,
  0b00011000,
  0b00110000,
  0b01111110,
  0b00000000,
};

static constexpr const uint8_t Char_3[] {
  0b00000000,
  0b00111100,
  0b01100110,
  0b00000110,
  0b00011100,
  0b01100110,
  0b00111100,
  0b00000000,
};

static constexpr const uint8_t Char_4[] {
  0b00000000,
  0b00001100,
  0b00011100,
  0b00101100,
  0b00101100,
  0b01111110,
  0b00001100,
  0b00000000,
};

static constexpr const uint8_t Char_5[] {
  0b00000000,
  0b01111110,
  0b01100000,
  0b01100000,
  0b01111100,
  0b00000110,
  0b01111100,
  0b00000000,
};

static constexpr const uint8_t Char_6[] {
  0b00000000,
  0b00111000,
  0b01100000,
  0b01100000,
  0b01111100,
  0b01100110,
  0b00111100,
  0b00000000,
};

static constexpr const uint8_t Char_7[] {
  0b00000000,
  0b01111110,
  0b01100110,
  0b00001100,
  0b00011000,
  0b00110000,
  0b00110000,
  0b00000000,
};

static constexpr const uint8_t Char_8[] {
  0b00000000,
  0b00111100,
  0b01100110,
  0b00111100,
  0b01100110,
  0b01100110,
  0b00111100,
  0b00000000,
};

static constexpr const uint8_t Char_9[] {
  0b00000000,
  0b00111100,
  0b01100110,
  0b01100110,
  0b00111110,
  0b00001100,
  0b00011000,
  0b00000000,
};

static constexpr const uint8_t Char_A[] {
  0b00000000,
  0b00011000,
  0b00111100,
  0b01100110,
  0b01111110,
  0b01100110,
  0b01100110,
  0b00000000,
};

static constexpr const uint8_t Char_B[] {
  0b00000000,
  0b01111100,
  0b01100110,
  0b01100100,
  0b01111110,
  0b01100110,
  0b01111100,
  0b00000000,
};

static constexpr const uint8_t Char_C[] {
  0b00000000,
  0b00111100,
  0b01100110,
  0b01100000,
  0b01100000,
  0b01100110,
  0b00111100,
  0b00000000,
};

static constexpr const uint8_t Char_D[] {
  0b00000000,
  0b01111100,
  0b01100110,
  0b01100110,
  0b01100110,
  0b01100110,
  0b01111100,
  0b00000000,
};

static constexpr const uint8_t Char_E[] {
  0b00000000,
  0b01111110,
  0b01100000,
  0b01111000,
  0b01100000,
  0b01100000,
  0b01111110,
  0b00000000,
};

static constexpr const uint8_t Char_F[] {
  0b00000000,
  0b01111110,
  0b01100000,
  0b01111000,
  0b01100000,
  0b01100000,
  0b01100000,
  0b00000000,
};

static constexpr const uint8_t Char_G[] {
  0b00000000,
  0b00111100,
  0b01100110,
  0b01100000,
  0b01101110,
  0b01100110,
  0b00111100,
  0b00000000,
};

static constexpr const uint8_t Char_H[] {
  0b00000000,
  0b01100110,
  0b01100110,
  0b01111110,
  0b01100110,
  0b01100110,
  0b01100110,
  0b00000000,
};

static constexpr const uint8_t Char_I[] {
  0b00000000,
  0b00011000,
  0b00011000,
  0b00011000,
  0b00011000,
  0b00011000,
  0b00011000,
  0b00000000,
};

static constexpr const uint8_t Char_J[] {
  0b00000000,
  0b00111110,
  0b00000110,
  0b00000110,
  0b00000110,
  0b01100110,
  0b00111000,
  0b00000000,
};

static constexpr const uint8_t Char_K[] {
  0b00000000,
  0b01100110,
  0b01101100,
  0b01111000,
  0b01111000,
  0b01101100,
  0b01100110,
  0b00000000,
};

static constexpr const uint8_t Char_L[] {
  0b00000000,
  0b01100000,
  0b01100000,
  0b01100000,
  0b01100000,
  0b01100000,
  0b01111110,
  0b00000000,
};

static constexpr const uint8_t Char_M[] {
  0b00000000,
  0b01000010,
  0b01100110,
  0b01111110,
  0b01010110,
  0b01000110,
  0b01000110,
  0b00000000,
};

static constexpr const uint8_t Char_N[] {
  0b00000000,
  0b01000110,
  0b01100110,
  0b01110110,
  0b01101110,
  0b01100110,
  0b01100010,
  0b00000000,
};

static constexpr const uint8_t Char_O[] {
  0b00000000,
  0b00111100,
  0b01100110,
  0b01100110,
  0b01100110,
  0b01100110,
  0b00111100,
  0b00000000,
};

static constexpr const uint8_t Char_P[] {
  0b00000000,
  0b01111100,
  0b01100110,
  0b01100110,
  0b01111100,
  0b01100000,
  0b01100000,
  0b00000000,
};

static constexpr const uint8_t Char_Q[] {
  0b00000000,
  0b00111100,
  0b01100110,
  0b01100110,
  0b01100110,
  0b01101110,
  0b00111011,
  0b00000000,
};

static constexpr const uint8_t Char_R[] {
  0b00000000,
  0b01111100,
  0b01100110,
  0b01100110,
  0b01111100,
  0b01101100,
  0b01100110,
  0b00000000,
};

static constexpr const uint8_t Char_S[] {
  0b00000000,
  0b00111100,
  0b01100110,
  0b00110000,
  0b00011100,
  0b01000110,
  0b00111100,
  0b00000000,
};

static constexpr const uint8_t Char_T[] {
  0b00000000,
  0b01111110,
  0b00011000,
  0b00011000,
  0b00011000,
  0b00011000,
  0b00011000,
  0b00000000,
};

static constexpr const uint8_t Char_U[] {
  0b00000000,
  0b01100110,
  0b01100110,
  0b01100110,
  0b01100110,
  0b01100110,
  0b00111100,
  0b00000000,
};

static constexpr const uint8_t Char_V[] {
  0b00000000,
  0b01100110,
  0b01100110,
  0b01100110,
  0b01100110,
  0b00111100,
  0b00011000,
  0b00000000,
};

static constexpr const uint8_t Char_W[] {
  0b00000000,
  0b01000110,
  0b01000110,
  0b01000110,
  0b01010110,
  0b01111110,
  0b01000110,
  0b00000000,
};

static constexpr const uint8_t Char_X[] {
  0b00000000,
  0b01100110,
  0b01100110,
  0b00111100,
  0b01100110,
  0b01100110,
  0b01100110,
  0b00000000,
};

static constexpr const uint8_t Char_Y[] {
  0b00000000,
  0b01100110,
  0b01100110,
  0b00111100,
  0b00011000,
  0b00011000,
  0b00011000,
  0b00000000,
};

static constexpr const uint8_t Char_Z[] {
  0b00000000,
  0b01111110,
  0b00000110,
  0b00001100,
  0b00011000,
  0b00110000,
  0b01111110,
  0b00000000,
};

static constexpr const uint8_t Char_v[] {
  0b00000000,
  0b00000000,
  0b00000000,
  0b11000110,
  0b01101100,
  0b00111000,
  0b00010000,
  0b00000000,
};

static constexpr const uint8_t Char_x[] {
  0b00000000,
  0b00000000,
  0b01100110,
  0b00111100,
  0b00011000,
  0b00111100,
  0b01100110,
  0b00000000,
};


static constexpr const uint8_t Char_z[] {
  0b00000000,
  0b00000000,
  0b00000000,
  0b01111100,
  0b00011000,
  0b00110000,
  0b01111100,
  0b00000000,
};

static constexpr const uint8_t Char_PERIOD[] {
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00110000,
  0b00110000,
  0b00000000,
};

static constexpr const uint8_t Char_COLON[] {
  0b00000000,
  0b00110000,
  0b00110000,
  0b00000000,
  0b00000000,
  0b00110000,
  0b00110000,
  0b00000000,
};

static constexpr const uint8_t Char_QUESTION[] {
  0b00000000,
  0b00111100,
  0b01100110,
  0b00001100,
  0b00011000,
  0b00000000,
  0b00011000,
  0b00000000,
};

static constexpr const uint8_t Char_EXCLAMATION[] {
  0b00000000,
  0b00110000,
  0b00110000,
  0b00110000,
  0b00110000,
  0b00000000,
  0b00110000,
  0b00000000,
};

static constexpr const uint8_t Char_AT[] {
  0b00000000,
  0b00111100,
  0b01000010,
  0b01011110,
  0b01011100,
  0b01000000,
  0b00111100,
  0b00000000,
};

static constexpr const uint8_t Char_SPACE[] {
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
};

static constexpr const uint8_t Char_GT[] {
  0b00000000,
  0b00100000,
  0b00010000,
  0b00001000,
  0b00000100,
  0b00001000,
  0b00010000,
  0b00100000,
};

static constexpr const uint8_t Char_PLUS[] {
  0b00000000,
  0b00000000,
  0b00010000,
  0b00010000,
  0b01111100,
  0b00010000,
  0b00010000,
  0b00000000,
};

static constexpr const uint8_t Char_MINUS[] {
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b01111100,
  0b00000000,
  0b00000000,
  0b00000000,
};

static constexpr const uint8_t Char_SQBR_OPEN[] {
  0b00000000,
  0b01111110,
  0b01000000,
  0b01000000,
  0b01000000,
  0b01000000,
  0b01111110,
  0b00000000,
};

static constexpr const uint8_t Char_SQBR_CLOSE[] {
  0b00000000,
  0b01111110,
  0b00000010,
  0b00000010,
  0b00000010,
  0b00000010,
  0b01111110,
  0b00000000,
};


// clang-format on

#endif // __BITMAPS_H__
