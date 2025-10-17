#include "sgp30.h"
#include <Arduino.h>
#include <math.h>

bool SGP30Sensor::begin() {
  Serial.println("[SGP30] Inicializando...");
  _ok = _sgp.begin(); // I2C 0x58
  Serial.println(_ok ? "[SGP30] OK" : "[SGP30] No detectado");
  return _ok;
}

float SGP30Sensor::absoluteHumidity_mg_m3(float tC, float rh) {
  if (!isfinite(tC) || !isfinite(rh)) return NAN;
  float vp = 6.112f * expf((17.62f * tC) / (243.12f + tC));
  float ah_g_m3 = 216.7f * ((rh / 100.0f) * vp) / (273.15f + tC);
  return ah_g_m3 * 1000.0f;
}

void SGP30Sensor::read(EnvData& io, float tempC, float humPct) {
  if (!_ok) return;
  io.hasCcs = true;

  float ah = absoluteHumidity_mg_m3(tempC, humPct);
  if (isfinite(ah)) _sgp.setHumidity((uint32_t)lroundf(ah));

  unsigned long now = millis();
  if (now - _lastIAQ < SGP_IAQ_MS) return;

  if (_sgp.IAQmeasure()) {
    io.eco2 = (float)_sgp.eCO2;
    io.tvoc = (float)_sgp.TVOC;
    Serial.printf("[SGP30] eCO2=%.0f ppm, TVOC=%.0f ppb\n", io.eco2, io.tvoc);
  } else {
    Serial.println("[SGP30] Lectura fallida");
  }

  _lastIAQ = now;
}
