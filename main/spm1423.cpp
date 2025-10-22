#include "spm1423.h"

#include <Arduino.h>
#include <cmath>
#include <driver/gpio.h>

namespace {
constexpr size_t kSampleCount = 512;
constexpr float kSilenceFloor = 1.0f;
}

bool SPM1423Sensor::begin(i2s_port_t port, int sampleRate, gpio_num_t clkPin, gpio_num_t dataPin) {
  end();

  _port = port;
  _sampleRate = sampleRate;

  i2s_chan_config_t chanConfig = I2S_CHANNEL_DEFAULT_CONFIG(_port, I2S_ROLE_MASTER);
  chanConfig.dma_desc_num = 4;
  chanConfig.dma_frame_num = 256;
  chanConfig.auto_clear = true;

  esp_err_t err = i2s_new_channel(&chanConfig, nullptr, &_rxChannel);
  if (err != ESP_OK) {
    Serial.printf("[SPM1423] Error creando canal PDM (%d)\n", (int)err);
    _rxChannel = nullptr;
    return false;
  }

  i2s_pdm_rx_config_t pdmConfig = {
      .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(sampleRate),
      .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
    .gpio_cfg = {
      .clk = static_cast<gpio_num_t>(clkPin),
      .din = static_cast<gpio_num_t>(dataPin),
    },
  };
  pdmConfig.slot_cfg.slot_mask = I2S_PDM_SLOT_LEFT;
  pdmConfig.slot_cfg.slot_mode = I2S_SLOT_MODE_MONO;

  err = i2s_channel_init_pdm_rx_mode(_rxChannel, &pdmConfig);
  if (err != ESP_OK) {
    Serial.printf("[SPM1423] Error configurando modo PDM (%d)\n", (int)err);
    end();
    return false;
  }

  err = i2s_channel_enable(_rxChannel);
  if (err != ESP_OK) {
    Serial.printf("[SPM1423] Error habilitando canal PDM (%d)\n", (int)err);
    end();
    return false;
  }

  _ok = true;
  Serial.println("[SPM1423] Microfono PDM listo");
  return true;
}

void SPM1423Sensor::read(EnvData& out) {
  out.hasNoise = false;
  out.noiseDb = NAN;
  if (!_ok || !_rxChannel) {
    return;
  }

  int16_t buffer[kSampleCount];
  size_t bytesRead = 0;
  esp_err_t err = i2s_channel_read(_rxChannel, buffer, sizeof(buffer), &bytesRead, 20 / portTICK_PERIOD_MS);
  if (err != ESP_OK || bytesRead == 0) {
    return;
  }

  size_t samples = bytesRead / sizeof(int16_t);
  if (samples == 0) {
    return;
  }

  double sumSquares = 0.0;
  for (size_t i = 0; i < samples; ++i) {
    float sample = static_cast<float>(buffer[i]);
    sumSquares += static_cast<double>(sample * sample);
  }

  double rms = std::sqrt(sumSquares / static_cast<double>(samples));
  if (rms < kSilenceFloor) {
    return;
  }

  float normalized = static_cast<float>(rms / 32767.0f);
  if (normalized <= 0.0f) {
    return;
  }

  float dbfs = 20.0f * static_cast<float>(std::log10(normalized));
  float dbSpl = 94.0f + dbfs;

  if (!std::isfinite(dbSpl)) {
    return;
  }

  out.hasNoise = true;
  out.noiseDb = dbSpl;
}

void SPM1423Sensor::end() {
  if (_rxChannel) {
    if (_ok) {
      i2s_channel_disable(_rxChannel);
    }
    i2s_del_channel(_rxChannel);
    _rxChannel = nullptr;
  }
  _ok = false;
}
