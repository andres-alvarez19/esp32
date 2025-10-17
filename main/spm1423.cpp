#include "spm1423.h"

#include <Arduino.h>
#include <cmath>
#include <cstring>
#include <esp_idf_version.h>

#ifndef ESP_IDF_VERSION_MAJOR
#define ESP_IDF_VERSION_MAJOR 3
#endif

namespace {
constexpr size_t kSampleCount = 512;
constexpr float kSilenceFloor = 1.0f;
}

bool SPM1423Sensor::begin(i2s_port_t port, int sampleRate, int bclkPin, int wsPin, int dataPin) {
  end();

  _port = port;
  _sampleRate = sampleRate;

  i2s_config_t config;
  std::memset(&config, 0, sizeof(config));
  config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
  config.sample_rate = sampleRate;
  config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  config.communication_format = I2S_COMM_FORMAT_STAND_MSB;
  config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  config.dma_buf_count = 4;
  config.dma_buf_len = 256;
  config.use_apll = false;
  config.tx_desc_auto_clear = false;
#if ESP_IDF_VERSION_MAJOR >= 4
  config.mclk_multiple = I2S_MCLK_MULTIPLE_DEFAULT;
  config.bits_per_chan = I2S_BITS_PER_SAMPLE_16BIT;
#endif

  if (i2s_driver_install(_port, &config, 0, nullptr) != ESP_OK) {
    Serial.println("[SPM1423] Error instalando driver I2S");
    return false;
  }

  i2s_pin_config_t pinConfig;
  std::memset(&pinConfig, 0, sizeof(pinConfig));
  pinConfig.bck_io_num = bclkPin;
  pinConfig.ws_io_num = wsPin;
  pinConfig.data_out_num = I2S_PIN_NO_CHANGE;
  pinConfig.data_in_num = dataPin;
#if ESP_IDF_VERSION_MAJOR >= 4
  pinConfig.mck_io_num = I2S_PIN_NO_CHANGE;
#endif

  if (i2s_set_pin(_port, &pinConfig) != ESP_OK) {
    Serial.println("[SPM1423] Error configurando pines I2S");
    i2s_driver_uninstall(_port);
    return false;
  }

  if (i2s_set_clk(_port, sampleRate, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO) != ESP_OK) {
    Serial.println("[SPM1423] Error ajustando reloj I2S");
    i2s_driver_uninstall(_port);
    return false;
  }

  i2s_zero_dma_buffer(_port);
  _ok = true;
  Serial.println("[SPM1423] Microfono listo");
  return true;
}

void SPM1423Sensor::read(EnvData& out) {
  out.hasNoise = false;
  out.noiseDb = NAN;
  if (!_ok) {
    return;
  }

  int16_t buffer[kSampleCount];
  size_t bytesRead = 0;
  esp_err_t err = i2s_read(_port, buffer, sizeof(buffer), &bytesRead, 20 / portTICK_PERIOD_MS);
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

  float normalized = static_cast<float>(rms / 32767.0);
  if (normalized <= 0.0f) {
    return;
  }

  float dbfs = 20.0f * static_cast<float>(std::log10(normalized));
  float dbSpl = 94.0f + dbfs;  // Conversión relativa (0 dBFS ≈ 94 dB SPL)

  if (!std::isfinite(dbSpl)) {
    return;
  }

  out.hasNoise = true;
  out.noiseDb = dbSpl;
}

void SPM1423Sensor::end() {
  if (_ok) {
    i2s_driver_uninstall(_port);
    _ok = false;
  }
}
