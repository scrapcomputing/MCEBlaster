//-*- C++ -*-
//
// Copyright (C) 2024 Scrap Computing
//

#ifndef __SRC_PICO_H__
#define __SRC_PICO_H__

#include "Utils.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "pico/stdlib.h"
#include <hardware/adc.h>
#include <iostream>
#include <map>
#include <memory>
#include <optional>

/// A range of GPIO pins. This is useful because it also creates the
/// corresponding mask which is used for setting and resetting pins.
class PinRange {
  uint32_t From = 0;
  uint32_t To = 0;
  uint32_t Mask = 0;

public:
  /// \p From is the first pin and \p To is the last pin in the reange.
  PinRange(uint32_t From, uint32_t To);
  /// Use this constructor to create a single pin.
  PinRange(uint32_t Pin) : PinRange(Pin, Pin) {}
  uint32_t getFrom() const { return From; }
  uint32_t getTo() const { return To; }
  uint32_t getMask() const { return Mask; }
  void dump(std::ostream &OS) const;
  DUMP_METHOD void dump() const;
};

class Pico {
  uint32_t State = 0;

public:
  static constexpr const uint16_t ADCMax = 1 << 12;
  Pico();
  /// Sets \p Pins to GPIO_OUT.
  inline void setGPIODirectionOut(const PinRange &Pins) {
    gpio_set_dir_out_masked(Pins.getMask());
  }
  /// Sets \p Pins to GPIO_IN.
  inline void setGPIODirectionIn(const PinRange &Pins) {
    gpio_set_dir_in_masked(Pins.getMask());
  }
  inline void setGPIODir(int GPIO, bool Out) {
    gpio_set_dir(GPIO, Out);
  }
  /// Sets \p Pins to \p Value.
  inline void setGPIOBits(const PinRange &Pins, uint32_t Value) {
    gpio_put_masked(Pins.getMask(), Value << Pins.getFrom());
  }
  /// Direction can be GPIO_IN GPIO_OUT.
  enum Pull {
    Up,
    Down,
    None,
  };
  void initGPIO(const PinRange &Pins, int Direction, Pull Pull, const char *Descr);
  void initADC(const PinRange &Pins, const char *Descr);
  /// Receives the status of all GPIO pins.
  inline void readGPIO() { State = gpio_get_all(); }
  /// \Returns the state of \p GPIO. Must have run `readGPIO()` first!
  inline bool getGPIO(uint32_t GPIO) const { return (State >> GPIO) & 0x1; }
  /// Sets \p GPIO to \p Value.
  inline void setGPIO(uint32_t GPIO, bool Value) { gpio_put(GPIO, Value); }
  inline bool getGPIO(uint32_t GPIO) { return gpio_get(GPIO); }
  /// Reads ADC at \p GPIO and returns the 12-bit value.
  /// Note: GPIO must be a valid ADC GPIO, i.e., 26-29.
  uint16_t readADC(uint32_t GPIO) const;
  void ledSet(bool State) { gpio_put(PICO_DEFAULT_LED_PIN, State); }
  void ledON() { gpio_put(PICO_DEFAULT_LED_PIN, 1); }
  void ledOFF() { gpio_put(PICO_DEFAULT_LED_PIN, 0); }
  inline void clear(uint32_t Mask) { gpio_clr_mask(Mask); }
  inline void set(uint32_t Mask) { gpio_set_mask(Mask); }
};

#endif // __SRC_PICO_H__
