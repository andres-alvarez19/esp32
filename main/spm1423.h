#pragma once
#include <Arduino.h>
#include <driver/i2s_pdm.h>

#include "env_data.h"

class SPM1423Sensor {
 public:
  bool begin();
  void read(EnvData& out);
  void end();

 private:
  static constexpr i2s_port_t PORT        = I2S_NUM_0;
  static constexpr int        SAMPLE_RATE = 16000;
  static constexpr int        BUF_SAMPLES = 512;
  static constexpr gpio_num_t PIN_CLK     = GPIO_NUM_26;
  static constexpr gpio_num_t PIN_DATA    = GPIO_NUM_27;

  i2s_chan_handle_t handle = nullptr;
  bool ok = false;
};
