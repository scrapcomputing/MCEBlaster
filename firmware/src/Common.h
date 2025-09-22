//-*- C++ -*-
//
// Copyright (C) 2024 Scarp Computing
//

#ifndef __COMMON_H__
#define __COMMON_H__

#include <cstdint>
static constexpr const int PXL_CLK_SMALL_STEP = 1000;
static constexpr const int PXL_CLK_STEP = 10000;

// Tolerate this much error in vertical Hz when auto-detecting the video mode.
static constexpr int AUTO_DETECT_MODE_V_HZ_ERROR = 1;

static constexpr const uint32_t VGA_RGB_GPIO = 14; // 14-19, 2 bits per color
static constexpr const uint32_t VGA_MDA_GPIO = 18;
static constexpr const uint32_t VGA_HSync_GPIO = 20;
static constexpr const uint32_t VGA_VSync_GPIO = 21;

static constexpr const uint32_t EGA_RGB_GPIO = 0; // 0-5:BBGGRR, 6:V 7:H
static constexpr const uint32_t TTL_VSYNC_GPIO = 6;
static constexpr const uint32_t TTL_HSYNC_GPIO = 7;
static constexpr const uint32_t CGA_RGB_GPIO = 6; // 6:V 7:H 8-13:BBGGRR
static constexpr const uint32_t CGA_ACTUAL_RGB_GPIO = 8; // 8-13:BBGGRR

static constexpr const uint32_t MDA_VI_GPIO = 26; // 26-27:IV (Intensity,Video)

static constexpr const uint32_t AUTO_ADJUST_GPIO = 28;
static constexpr const uint32_t PX_CLK_BTN_GPIO = 22;

static constexpr const int DEBOUNCE_SZ = 2;
static constexpr const int LONGPRESS_CNT = 60;

/// Cancel pixel clock adjustment after this many frames of inactivity.
static constexpr const uint32_t PX_CLK_END_TIME_MS = 12000;
static constexpr const int PX_CLK_INITIAL_TXT_DISPLAY_MS = PX_CLK_END_TIME_MS;
static constexpr const int PX_CLK_TXT_DISPLAY_MS = PX_CLK_END_TIME_MS;
static constexpr const int PX_CLK_EXIT_DISPLAY_MS = 2000;
static constexpr const int AUTO_ADJUST_DISPLAY_MS = 150;
static constexpr const int AUTO_ADJUST_ALWAYS_ON_DISPLAY_MS = 2000;

static constexpr const int RESET_TO_DEFAULTS_DISPLAY_MS = 1000;

static constexpr const uint32_t MANUAL_TTL_TIMEOUT_MS = 12000;
static constexpr const int MANUAL_TTL_DISPLAY_MS = MANUAL_TTL_TIMEOUT_MS - 200;
static constexpr const uint32_t MANUAL_TTL_DISPLAY_DONE_MS = 1000;
static constexpr const uint32_t MANUAL_TTL_DISPLAY_SAVED_MS = 3000;
static constexpr const uint32_t MANUAL_TTL_HORIZ_STEP = 1;
static constexpr const uint32_t MANUAL_TTL_VERT_STEP = 1;
static constexpr const uint32_t MANUAL_TTL_HORIZ_MIN = 100;
static constexpr const uint32_t MANUAL_TTL_VERT_MIN = 100;
static constexpr const int MANUAL_TTL_XBORDER_STEP = 4;
static constexpr const int MANUAL_TTL_YBORDER_STEP = 1;
static constexpr const int MANUAL_TTL_MAX_XBORDER = 400;
static constexpr const int MANUAL_TTL_MAX_YBORDER = 200;

static constexpr const uint32_t NO_TTL_SIGNAL_MS = 800;
static constexpr const uint32_t PROFILE_DISPLAY_MS = 4000;
static constexpr const uint32_t UNKNOWN_MODE_MS = 2000;
/// Limit the number of times we will show the "unknown mode" message.
static constexpr const uint32_t UNKNOWN_MODE_SHOW_MSG_MAX_COUNT = 3;
/// Wait for a bit until we print the "unknown mode message"
static constexpr const uint32_t UNKNOWN_MODE_SHOW_MSG_MIN_COUNT = 2;
static constexpr const int TTL_MOD = 4;

static constexpr const uint32_t LED_FRAME_MOD = 128;
static constexpr const uint32_t LED_MOD_ON = 0;
static constexpr const uint32_t LED_MOD_OFF = 64;

/// Adjust every this many frames to avoid artifacts.
static constexpr const uint32_t AUTO_ADJUST_THROTTLE_CNT = 4;
/// Reset borders after this many frames. Helpful for the single-ON operation.
static constexpr const uint32_t AUTO_ADJUST_START_AFTER = 2;
/// Duration in frames.
static constexpr const uint32_t AUTO_ADJUST_DURATION = 16;

/// Button LongPress cnt (in frames)
static constexpr const int BTN_LONG_PRESS_FRAMES = 60;


static constexpr const uint32_t Black = 0b000000;
static constexpr const uint32_t White = 0b111111;
static constexpr const uint32_t Red = 0b110000;
static constexpr const uint32_t Green = 0b001100;
static constexpr const uint32_t DarkGreen = 0b000100;
static constexpr const uint32_t Blue = 0b000011;
static constexpr const uint32_t Gray = 0b101010;
static constexpr const uint32_t DarkGray = 0b010101;
static constexpr const uint32_t BrightCyan = 0b011111;



static constexpr const uint32_t HMask = 0b01000000;
static constexpr const uint32_t VMask = 0b10000000;

static constexpr const uint32_t VGADarkYellow = 0b101000 | HMask; // VHRRGGBB
static constexpr const uint32_t VGABrown      = 0b100100 | HMask;

static constexpr const uint32_t HMask_4 =
    HMask | HMask << 8 | HMask << 16 | HMask << 24;
static constexpr const uint32_t VMask_4 =
    VMask | VMask << 8 | VMask << 16 | VMask << 24;

static constexpr const uint32_t HVMask = HMask | VMask;
static constexpr const uint32_t HVMask_4 = HMask_4 | VMask_4;

static constexpr const uint32_t Black_4 =
    Black | Black << 8 | Black << 16 | Black << 24;

static constexpr const uint32_t Black4_HV = Black_4 | HVMask_4;
static constexpr const uint32_t Black4_V = Black_4 | VMask_4;


static constexpr const uint32_t BlackMDA = 0u;
static constexpr const uint32_t BlackMDA_8 = 0u;
static constexpr const uint32_t HMaskMDA = 0b0100;
static constexpr const uint32_t VMaskMDA = 0b1000;
static constexpr const uint32_t HMaskMDA_8 =
    HMaskMDA | HMaskMDA << 4 | HMaskMDA << 8 | HMaskMDA << 12 | HMaskMDA << 16 |
    HMaskMDA << 20 | HMaskMDA << 24 | HMaskMDA << 28;
static constexpr const uint32_t VMaskMDA_8 =
    VMaskMDA | VMaskMDA << 4 | VMaskMDA << 8 | VMaskMDA << 12 | VMaskMDA << 16 |
    VMaskMDA << 20 | VMaskMDA << 24 | VMaskMDA << 28;
static constexpr const uint32_t HVMaskMDA_8 = HMaskMDA_8 | VMaskMDA_8;
static constexpr const uint32_t BlackMDA_8_HV =
    BlackMDA_8 | HMaskMDA_8 | VMaskMDA_8;
static constexpr const uint32_t BlackMDA_8_V = BlackMDA_8 | VMaskMDA_8;

static constexpr const uint32_t MDAPixels = 800;
static constexpr const uint32_t MDALines = 600;

static constexpr const uint32_t TTLVMask = 1u << TTL_VSYNC_GPIO;
static constexpr const uint32_t TTLVSyncMask =
    TTLVMask | TTLVMask << 8 | TTLVMask << 16 | TTLVMask << 24;

static constexpr const uint32_t TTLHMask = 1u << TTL_HSYNC_GPIO;
static constexpr const uint32_t TTLHSyncMask =
    TTLHMask | TTLHMask << 8 | TTLHMask << 16 | TTLHMask << 24;
static constexpr const uint32_t RGBMask = 0b111111;
static constexpr const uint32_t RGBMask_4 =
    RGBMask | RGBMask << 8 | RGBMask << 16 | RGBMask << 24;

#endif // __COMMON_H__
