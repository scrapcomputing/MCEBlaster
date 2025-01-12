cmake_minimum_required(VERSION 3.13)

# Build
# -----
# $ mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -DPICO_FREQ=270000 -DPICO_SDK_PATH=/path/to/pico-sdk/ ../src/ && make -j
#
# Options NOTE: don't use 'off' to disable, just remove the argument.
# -------
# o -DFULL_FLASH_FREQ=on to disable reducing the flash frequency
# o -DDISABLE_PICO_LED=on to disable the Pico's blinking LED.
# o -DDBGPRINT=on to enable debug messages
# o -DPICO_FREQ=<KHz> to set the Pico's frequency
# o -DPICO_VOLTAGE=<voltage> VREG_VOLTAGE_1_10 (=1.10v) is the default

# Some valid frequencies: 225000, 250000, 270000, 280000, 290400
# Voltages: <pico-sdk>/src/rp2_common/hardware_vreg/include/hardware/vreg.h
#           Examples:  VREG_VOLTAGE_0_85 0.85v
#                      VREG_VOLTAGE_1_30 1.30v

#
# This will place the firmware into: build/MCEBlaster.uf2
#
# For example: minicom -b 115200 -D /dev/ttyACM0. Serial connection: 115200 8N1

set(REVISION_MAJOR 0)
set(REVISION_MINOR 1)

message("PICO_SDK_PATH = ${PICO_SDK_PATH}")

# set(PICO_EXTRAS_PATH "$ENV{HOME}/pico-extras/")

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

set(PROJECT_NAME MCEBlaster)
project(
  ${PROJECT_NAME}
  LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()
include_directories("${PICO_SDK_PATH}/src/common/pico_stdlib/include")
include_directories("${PICO_SDK_PATH}/src/common/pico_base/include")
include_directories("${PICO_SDK_PATH}/src/rp2_common/hardware_adc/include")
include_directories("${PICO_SDK_PATH}/src/rp2_common/pico_multicore/include")
include_directories("${PROJECT_BINARY_DIR}/") # for build/config.h

set(CMAKE_CXX_FLAGS_RELEASE "-Os")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Werror ${EXTRA_DBG_FLAGS}")

# rest of your project
file(GLOB SOURCES *.c *.cpp *.h *.def)
add_executable(${PROJECT_NAME} ${SOURCES})

# The 270MHz frequency may be too high for the flash, so use a divider
if (NOT DEFINED FULL_FLASH_FREQ)
   message("-> Using reduced flash frequency!")
   pico_define_boot_stage2(slower_boot2 ${PICO_DEFAULT_BOOT_STAGE2_FILE})
   target_compile_definitions(slower_boot2 PRIVATE PICO_FLASH_SPI_CLKDIV=4)
   pico_set_boot_stage2(${PROJECT_NAME} slower_boot2)
endif ()

include(EGAPio.cmake)
include(CGAPio.cmake)
include(MDAPio.cmake)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/Pio/VGAOut4x1Pixels.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/Pio/VGAOut8x1MDA.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/Pio/EGA640x350Border.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/Pio/CGA640x200.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/Pio/CGA640x200Border.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/Pio/MDA720x350.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/Pio/MDA720x350Border.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/Pio/NoInputSignal.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/Pio/VSyncPolarity.pio)

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

set(LIBS
  pico_stdlib
  pico_multicore
  hardware_pio
  )
target_link_libraries(${PROJECT_NAME} ${LIBS})

# Use -DPICO_FREQ to override the default frequency 133000
# *** Please note that not all frequency values are valid
#     Some good values: 125000 133000 225000 270000
set(PICO_DEFAULT_FREQ 125000)
if (NOT DEFINED PICO_FREQ)
  set(PICO_FREQ ${PICO_DEFAULT_FREQ})
endif ()

set(PICO_DEFAULT_VOLTAGE VREG_VOLTAGE_1_10)
if (NOT DEFINED PICO_VOLTAGE)
  set(PICO_VOLTAGE ${PICO_DEFAULT_VOLTAGE})
endif ()


message("")
message("+---------------+")
message("| Configuration |")
message("+---------------+")
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
if (DISABLE_USB_DBG STREQUAL "0")
  pico_enable_stdio_usb(${PROJECT_NAME} 0)
  pico_enable_stdio_uart(${PROJECT_NAME} 0)
endif ()
message("DISABLE_USB_DBG = ${DISABLE_USB_DBG}")

message("PICO_LED = ${PICO_LED}")
message("PICO_FREQ = ${PICO_FREQ} (KHz)")
message("PICO_VOLTAGE = ${PICO_VOLTAGE}")
message("FULL_FLASH_FREQ = ${FULL_FLASH_FREQ}")


# End of configuration
message("")

configure_file (
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
  )


# Create map/bin/hex/uf2 in addition to ELF.
pico_add_extra_outputs(${PROJECT_NAME})
