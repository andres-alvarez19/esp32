#include "bme280.h"
#include <Arduino.h>

bool BME280Sensor::begin(uint8_t addrPrimary, uint8_t addrAlt) {
  Serial.println("[BME280] Buscando...");
  _ok = _bme.begin(addrPrimary) || _bme.begin(addrAlt);
  Serial.println(_ok ? "[BME280] OK" : "[BME280] No detectado");
  return _ok;
}

void BME280Sensor::read(EnvData& out) {
  if (!_ok) return;
  out.hasBme = true;
  out.temp = _bme.readTemperature();
  out.hum  = _bme.readHumidity();
  out.press = _bme.readPressure() / 100.0F;
  out.alt   = _bme.readAltitude(1013.25);
}
