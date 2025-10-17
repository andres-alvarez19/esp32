#pragma once
#include <BH1750.h>
#include "env_data.h"

class BH1750Sensor {
 public:
  bool begin(uint8_t addrPrimary = 0x23, uint8_t addrAlt = 0x5C);
  void read(EnvData& out);

 private:
  BH1750 _bh;
  bool _ok = false;
};
