#include "bh1750.h"
#include <Arduino.h>
#include <math.h>

bool BH1750Sensor::begin(uint8_t addrPrimary, uint8_t addrAlt) {
  Serial.println("[BH1750] Buscando...");
  _ok = _bh.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, addrPrimary) ||
        _bh.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, addrAlt);
  Serial.println(_ok ? "[BH1750] OK" : "[BH1750] No detectado");
  return _ok;
}

void BH1750Sensor::read(EnvData& out) {
  out.hasLight = false;
  out.lux = NAN;
  if (!_ok) return;
  float lux = _bh.readLightLevel();
  if (isfinite(lux)) {
    out.hasLight = true;
    out.lux = lux;
  }
}
