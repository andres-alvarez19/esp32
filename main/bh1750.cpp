#include "bh1750.h"
#include <Arduino.h>
#include <Wire.h>
#include <math.h>

namespace {
bool probeI2C(uint8_t addr) {
  // Asegura que el bus esté inicializado antes de probar la dirección
  Wire.begin();
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}
}

bool BH1750Sensor::begin(uint8_t addrPrimary, uint8_t addrAlt) {
  Serial.println("[BH1750] Buscando...");

  uint8_t addr = 0;
  if (probeI2C(addrPrimary)) {
    addr = addrPrimary;
  } else if (probeI2C(addrAlt)) {
    addr = addrAlt;
  } else {
    Serial.println("[BH1750] No detectado (sin respuesta I2C)");
    _ok = false;
    return false;
  }

  _ok = _bh.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, addr);
  Serial.println(_ok ? "[BH1750] OK" : "[BH1750] No detectado");
  return _ok;
}

void BH1750Sensor::read(EnvData& out) {
  out.hasLight = false;
  out.lux = NAN;
  if (!_ok) return;
  float lux = _bh.readLightLevel();
  if (isfinite(lux) && lux >= 0.0f) {
    out.hasLight = true;
    out.lux = lux;
  }
}
