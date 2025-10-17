#pragma once

#include <driver/i2s.h>
#include "env_data.h"
#include "pins.h"

class SPM1423Sensor {
 public:
  bool begin(i2s_port_t port = I2S_NUM_0,
             int sampleRate = 16000,
             int bclkPin = MIC_BCLK_PIN,
             int wsPin = MIC_WS_PIN,
             int dataPin = MIC_DATA_PIN);
  void read(EnvData& out);
  void end();

 private:
  bool _ok = false;
  i2s_port_t _port = I2S_NUM_0;
  int _sampleRate = 16000;
};
