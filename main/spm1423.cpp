#include "spm1423.h"
#include <cmath>
#include <limits.h>

bool SPM1423Sensor::begin() {
  end();  // limpiar si quedó algo previo

  i2s_chan_config_t cfg = I2S_CHANNEL_DEFAULT_CONFIG(PORT, I2S_ROLE_MASTER);
  cfg.auto_clear    = true;
  cfg.dma_desc_num  = 4;
  cfg.dma_frame_num = 256;

  if (i2s_new_channel(&cfg, nullptr, &handle) != ESP_OK) {
    handle = nullptr;
    return false;
  }

  i2s_pdm_rx_config_t pdm = {
    .clk_cfg  = I2S_PDM_RX_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
    .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(
                  I2S_DATA_BIT_WIDTH_16BIT,
                  I2S_SLOT_MODE_MONO),
    .gpio_cfg = {}
  };

  pdm.slot_cfg.slot_mask = I2S_PDM_SLOT_LEFT;
  pdm.gpio_cfg.clk = PIN_CLK;
  pdm.gpio_cfg.din = PIN_DATA;

  if (i2s_channel_init_pdm_rx_mode(handle, &pdm) != ESP_OK) {
    end();
    return false;
  }

  if (i2s_channel_enable(handle) != ESP_OK) {
    end();
    return false;
  }

  ok = true;
  return true;
}

void SPM1423Sensor::read(EnvData& out) {
  out.hasNoise = false;
  out.noiseDb  = NAN;
  if (!ok || !handle) return;

  int16_t buf[BUF_SAMPLES];
  size_t br = 0;

  if (i2s_channel_read(handle, buf, sizeof(buf), &br,
                       50 / portTICK_PERIOD_MS) != ESP_OK || br == 0) {
    return;
  }

  int n = br / sizeof(int16_t);
  if (n <= 0) return;

  // Calcular RMS y spread
  double sum = 0.0;
  int16_t minv = INT16_MAX;
  int16_t maxv = INT16_MIN;

  for (int i = 0; i < n; i++) {
    int16_t v = buf[i];
    if (v < minv) minv = v;
    if (v > maxv) maxv = v;
    sum += (double)v * (double)v;
  }

  int spread = maxv - minv;

  // Filtro básico de silencio o línea plana
  if (spread < 5) {
    return;
  }

  // RMS normalizado 0..1 (dBFS depende de esto)
  float rmsNorm = sqrt(sum / n) / 32767.0f;
  if (rmsNorm <= 0.0f) return;

  // Convertir a dBFS
  float dbfs = 20.0f * log10f(rmsNorm);

  // Convertir a SPL aproximado (no calibrado)
  float spl = 94.0f + dbfs;
  if (!std::isfinite(spl)) return;

  // === Índice normalizado 0–100 ===
  //
  // Valores prácticos (ajustables):
  //  - quietRef = nivel en ambiente silencioso
  //  - loudRef  = nivel que consideras "muy fuerte"
  //
  // Estos valores no afectan a la lectura real,
  // solo al índice expresado al usuario.
  //
  float quietRef = 55.0f;   // sala tranquila (~50–55 dB)
  float loudRef  = 90.0f;   // ruido fuerte (~80–90 dB)

  float idx = (spl - quietRef) / (loudRef - quietRef) * 100.0f;

  if (idx < 0.0f) idx = 0.0f;
  if (idx > 100.0f) idx = 100.0f;

  out.hasNoise = true;
  out.noiseDb  = idx;   // <<< ahora es índice normalizado 0–100
}

void SPM1423Sensor::end() {
  if (handle) {
    i2s_channel_disable(handle);
    i2s_del_channel(handle);
    handle = nullptr;
  }
  ok = false;
}
