#pragma once

#include <driver/i2s_common.h>
#include <driver/i2s_pdm.h>
#include "env_data.h"
#include "pins.h"

class SPM1423Sensor {
 public:
  bool begin(i2s_port_t port = I2S_NUM_0,
             int sampleRate = 16000,
             int clkPin = MIC_CLK_PIN,
             int dataPin = MIC_DATA_PIN);
  void read(EnvData& out);
  void end();

 private:
  bool _ok = false;
  i2s_port_t _port = I2S_NUM_0;
  i2s_chan_handle_t _rxChannel = nullptr;
  int _sampleRate = 16000;
};
