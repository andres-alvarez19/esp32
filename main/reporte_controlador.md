# Informe de código del controlador

## Arquitectura general
- `main.ino` inicializa el hardware (buzzer, LEDs) y delega el ciclo de trabajo en la clase `App`.
- `App` gestiona el ciclo de adquisición de sensores, el formateo de datos y la publicación en la nube.
- El driver `SPM1423Sensor` implementa la captura PDM y calcula un nivel de ruido estimado en dB SPL.
- Las estructuras auxiliares (`EnvData`, `pins.h`) mantienen la información compartida y el mapeado de pines.

## Controlador principal (`main.ino`)
```cpp
#include <Arduino.h>
#include <cmath>

#include "app.h"
#include "buzzer.h"
#include "pins.h"

namespace {
constexpr float kCo2AlertThreshold = 1500.0f;
}

App app;
ActiveBuzzer buzzer;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== Proyecto ESP32 – Monitor Ambiental ===");

  pinMode(LED_VERDE_PIN, OUTPUT);
  pinMode(LED_ROJO_PIN, OUTPUT);
  digitalWrite(LED_VERDE_PIN, LOW);
  digitalWrite(LED_ROJO_PIN, LOW);

  bool buzzerReady = buzzer.begin();
  String failed = "";
  if (!buzzerReady) {
    failed = "Buzzer";
    Serial.println("[MAIN] Error al iniciar Buzzer");
  }

  bool modulesOk = app.begin();
  if (!modulesOk) {
    if (!failed.isEmpty()) failed += ", ";
    failed += app.failedModules();
  }

  if (failed.isEmpty()) {
    Serial.println("[MAIN] Monitor listo");
    digitalWrite(LED_VERDE_PIN, HIGH);
    digitalWrite(LED_ROJO_PIN, LOW);
  } else {
    Serial.print("[MAIN] Monitor inicializado con errores: ");
    Serial.println(failed);
    digitalWrite(LED_ROJO_PIN, HIGH);
    digitalWrite(LED_VERDE_PIN, LOW);
  }

  buzzer.off();
}

void loop() {
  app.update();

  bool sensorsReady = app.modulesReady();
  bool buzzerReady = buzzer.isReady();

  if (!sensorsReady || !buzzerReady) {
    digitalWrite(LED_ROJO_PIN, HIGH);
    digitalWrite(LED_VERDE_PIN, LOW);
    buzzer.off();
    return;
  }

  const EnvData& data = app.data();
  bool alert = std::isfinite(data.eco2) && data.eco2 > kCo2AlertThreshold;

  if (alert) {
    digitalWrite(LED_ROJO_PIN, HIGH);
    digitalWrite(LED_VERDE_PIN, LOW);
    buzzer.on();
  } else {
    digitalWrite(LED_ROJO_PIN, LOW);
    digitalWrite(LED_VERDE_PIN, HIGH);
    buzzer.off();
  }
}
```

## Orquestador de sensores (`app.h` y `app.cpp`)

### app.h
```cpp
#pragma once
#include <Arduino.h>

#include "config.h"
#include "env_data.h"
#include "bme280.h"
#include "sgp30.h"
#include "bh1750.h"
#include "spm1423.h"
#include "oled.h"
#include "ubidots.h"

class App {
 public:
  bool begin();
  void update();
  bool modulesReady() const { return _modulesReady; }
  const EnvData& data() const { return _data; }
  const String& failedModules() const { return _failedModules; }

 private:
  BME280Sensor _bme;
  SGP30Sensor  _sgp;
  BH1750Sensor _light;
  SPM1423Sensor _sound;
  OledView      _oled;
  UbidotsClient _ubi;
  EnvData _data;

  unsigned long _lastPublish = 0;
  bool _modulesReady = false;
  String _failedModules;
};
```

### app.cpp
```cpp
#include "app.h"
#include <Arduino.h>

bool App::begin() {
  _modulesReady = false;
  _failedModules = "";

  bool modulesOk = true;
  auto checkModule = [&](const char* name, bool ok) {
    if (ok) {
      Serial.printf("[APP] %s inicializado\n", name);
    } else {
      if (!_failedModules.isEmpty()) _failedModules += ", ";
      _failedModules += name;
      Serial.printf("[APP] Error al iniciar %s\n", name);
      modulesOk = false;
    }
  };

  checkModule("BME280", _bme.begin());
  checkModule("SGP30", _sgp.begin());
  checkModule("BH1750", _light.begin());
  checkModule("SPM1423", _sound.begin());
  checkModule("OLED", _oled.begin());
  checkModule("Ubidots", _ubi.begin());

  _modulesReady = modulesOk;
  if (!_modulesReady) {
    Serial.print("[APP] Modulos con fallo: ");
    Serial.println(_failedModules);
  } else {
    _lastPublish = millis();
  }

  return _modulesReady;
}

void App::update() {
  _ubi.loop();

  if (!_modulesReady) {
    return;
  }

  _bme.read(_data);
  _sgp.read(_data, _data.temp, _data.hum);
  _light.read(_data);
  _sound.read(_data);

  if (millis() - _lastPublish > 5000) {
    auto printFloat = [](const char* name, float v) {
      if (isnan(v)) {
        Serial.printf("%s: N/A\n", name);
      } else {
        Serial.printf("%s: %.2f\n", name, v);
      }
    };

    Serial.println("[APP] Lectura sensores:");
    Serial.printf("  BME presente: %s\n", _data.hasBme ? "si" : "no");
    printFloat("    Temp (C)", _data.temp);
    printFloat("    Hum (%)", _data.hum);
    printFloat("    Press (hPa)", _data.press);
    printFloat("    Alt (m)", _data.alt);

    Serial.printf("  SGP30 (gas) presente: %s\n", _data.hasCcs ? "si" : "no");
    printFloat("    eCO2 (ppm)", _data.eco2);
    printFloat("    TVOC (ppb)", _data.tvoc);

    Serial.printf("  BH1750 presente: %s\n", _data.hasLight ? "si" : "no");
    printFloat("    Lux", _data.lux);

    Serial.printf("  Microfono presente: %s\n", _data.hasNoise ? "si" : "no");
    printFloat("    Noise dB SPL", _data.noiseDb);

    _ubi.addEnv(_data);
    _ubi.publish();
    _oled.draw(_data, 2000.0f);
    _lastPublish = millis();
  }
}
```

## Sensor de ruido PDM (`spm1423.h` y `spm1423.cpp`)

### spm1423.h
```cpp
#pragma once

#include <driver/i2s_common.h>
#include <driver/i2s_pdm.h>
#include <driver/gpio.h>
#include "env_data.h"
#include "pins.h"

class SPM1423Sensor {
 public:
  bool begin(i2s_port_t port = I2S_NUM_0,
             int sampleRate = 16000,
             gpio_num_t clkPin = static_cast<gpio_num_t>(MIC_CLK_PIN),
             gpio_num_t dataPin = static_cast<gpio_num_t>(MIC_DATA_PIN));
  void read(EnvData& out);
  void end();

 private:
  bool _ok = false;
  i2s_port_t _port = I2S_NUM_0;
  i2s_chan_handle_t _rxChannel = nullptr;
  int _sampleRate = 16000;
};
```

### spm1423.cpp
```cpp
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
```

## Estructuras de apoyo

### env_data.h
```cpp
#pragma once
#include <math.h>

struct EnvData {
  bool hasBme = false;
  bool hasCcs = false;
  bool hasLight = false;
  bool hasNoise = false;
  float temp = NAN;
  float hum  = NAN;
  float press = NAN;
  float alt   = NAN;
  float eco2  = NAN;
  float tvoc  = NAN;
  float lux   = NAN;
  float noiseDb = NAN;
};
```

### pins.h
```cpp
#pragma once
#define LED_VERDE_PIN 13
#define LED_ROJO_PIN  14
#define BUZZER_PIN    25
#define MIC_CLK_PIN   26
#define MIC_DATA_PIN  34
```
