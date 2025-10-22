#include "buzzer.h"

#include <Arduino.h>

bool ActiveBuzzer::begin(uint8_t pin) {
  _pin = pin;
  if (_pin >= 40) {
    Serial.println("[Buzzer] Pin invalido");
    _ready = false;
    _active = false;
    return false;
  }

  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
  _ready = true;
  _active = false;
  Serial.println("[Buzzer] Listo");
  return true;
}

void ActiveBuzzer::on() {
  if (!_ready) return;
  digitalWrite(_pin, HIGH);
  _active = true;
}

void ActiveBuzzer::off() {
  if (!_ready) return;
  digitalWrite(_pin, LOW);
  _active = false;
}

void ActiveBuzzer::set(bool enable) {
  if (enable) {
    on();
  } else {
    off();
  }
}
