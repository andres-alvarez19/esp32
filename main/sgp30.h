#pragma once
#include <Adafruit_SGP30.h>
#include "env_data.h"

class SGP30Sensor {
 public:
  bool begin();
  void read(EnvData& io, float tempC, float humPct);

 private:
  Adafruit_SGP30 _sgp;
  bool _ok = false;
  unsigned long _lastIAQ = 0;
  static constexpr uint32_t SGP_IAQ_MS = 1000UL;
  static float absoluteHumidity_mg_m3(float tempC, float rhPct);
};
