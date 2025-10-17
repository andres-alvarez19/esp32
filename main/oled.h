#pragma once
#include <Adafruit_SSD1306.h>
#include "env_data.h"

class OledView {
 public:
  bool begin(uint8_t addr = 0x3C);
  void draw(const EnvData& d, float co2MaxPpm);

 private:
  Adafruit_SSD1306 _disp{128, 64, &Wire, -1};
  bool _ok = false;
};
