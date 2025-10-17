#pragma once
#include <math.h>

struct EnvData {
  bool hasBme = false;
  bool hasCcs = false;   // mantiene compatibilidad como "sensor gas presente"
  float temp = NAN;
  float hum  = NAN;
  float press = NAN;
  float alt   = NAN;
  float eco2  = NAN;     // ppm
  float tvoc  = NAN;     // ppb
};
