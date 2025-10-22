#pragma once
#include <Arduino.h>

#include "config.h"
#include "env_data.h"
#include "bme280.h"
#include "sgp30.h"
#include "bh1750.h"
#include "spm1423.h"
#include "oled.h"
#include "ubidots.h"

class App {
 public:
  bool begin();
  void update();
  bool modulesReady() const { return _modulesReady; }
  const EnvData& data() const { return _data; }
  const String& failedModules() const { return _failedModules; }

 private:
  BME280Sensor _bme;
  SGP30Sensor  _sgp;
  BH1750Sensor _light;
  SPM1423Sensor _sound;
  OledView      _oled;
  UbidotsClient _ubi;
  EnvData _data;

  unsigned long _lastPublish = 0;
  bool _modulesReady = false;
  String _failedModules;
};
