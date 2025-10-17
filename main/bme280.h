#pragma once
#include <Adafruit_BME280.h>
#include "env_data.h"

class BME280Sensor {
 public:
  bool begin(uint8_t addrPrimary = 0x77, uint8_t addrAlt = 0x76);
  void read(EnvData& out);
 private:
  Adafruit_BME280 _bme;
  bool _ok = false;
};
