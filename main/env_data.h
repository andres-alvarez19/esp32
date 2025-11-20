#pragma once
#include <math.h>

struct EnvData {
  bool hasBme = false;
  bool hasCcs = false;   
  bool hasLight = false;
  bool hasNoise = false;
  float temp = NAN;
  float hum  = NAN;
  float eco2  = NAN;   
  float tvoc  = NAN;    
  float lux   = NAN;     
  float noiseDb = NAN;   
};
