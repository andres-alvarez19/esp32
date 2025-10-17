#pragma once
#include <math.h>

struct EnvData {
  bool hasBme = false;
  bool hasCcs = false;   // mantiene compatibilidad como "sensor gas presente"
  bool hasLight = false;
  bool hasNoise = false;
  float temp = NAN;
  float hum  = NAN;
  float press = NAN;
  float alt   = NAN;
  float eco2  = NAN;     // ppm
  float tvoc  = NAN;     // ppb
  float lux   = NAN;     // lux
  float noiseDb = NAN;   // dB SPL estimados
};
