#pragma once
#include "config.h"
#include "pins.h"
#include "env_data.h"
#include "bme280.h"
#include "sgp30.h"
#include "oled.h"
#include "ubidots.h"

class App {
 public:
  void begin();
  void loop();

 private:
  BME280Sensor _bme;
  SGP30Sensor  _sgp;
  OledView     _oled;
  UbidotsClient _ubi;
  EnvData _data;

  unsigned long _lastPublish = 0;
};
