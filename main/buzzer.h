#pragma once

#include <stdint.h>

#include "pins.h"

class ActiveBuzzer {
 public:
  bool begin(uint8_t pin = BUZZER_PIN);
  void on();
  void off();
  void set(bool enable);
  bool isReady() const { return _ready; }
  bool isActive() const { return _active; }

 private:
  uint8_t _pin = BUZZER_PIN;
  bool _ready = false;
  bool _active = false;
};
