# Informe de código del controlador

## Arquitectura general
- `main.ino` inicializa el hardware (buzzer, LEDs) y delega el ciclo de trabajo en la clase `App`.
- `App` gestiona el ciclo de adquisición de sensores, el formateo de datos y la publicación en la nube.
- El driver `SPM1423Sensor` implementa la captura PDM y calcula un nivel de ruido estimado en dB SPL.
- Las estructuras auxiliares (`EnvData`, `pins.h`) mantienen la información compartida y el mapeado de pines.
- `UbidotsClient` se encarga de la conectividad WiFi, el empaquetado de lecturas y la publicación MQTT hacia Ubidots.

## Controlador principal (`main.ino`)
Gestiona el ciclo de vida del firmware: arranca los periféricos, valida qué módulos se inicializaron correctamente y decide acciones visibles (LEDs, zumbador) en función de las lecturas ambientales publicadas por `App`.
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
Declara la clase `App`, responsable de abstraer cada sensor y servicios asociados. Mantiene el estado compartido (`EnvData`), las instancias de cada módulo y expone métodos para iniciar y actualizar el conjunto completo.
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
Implementa la lógica de inicialización secuencial y la actualización periódica: invoca a cada sensor para poblar `EnvData`, imprime diagnósticos, dibuja en la pantalla OLED y envía los datos a Ubidots a intervalos fijos.
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

## Cliente Ubidots (`ubidots.h` y `ubidots.cpp`)

### ubidots.h
Declara el contenedor del cliente MQTT usado para conectarse a Ubidots. Se inicializa con el token configurado y ofrece métodos para preparar la sesión, cargar lecturas y publicar.
```cpp
#pragma once
#include <UbidotsEsp32Mqtt.h>
#include "config.h"
#include "env_data.h"

class UbidotsClient {
 public:
  explicit UbidotsClient(const char* token = UBIDOTS_TOKEN)
  : _ubi(token, UBIDOTS_HOST, UBIDOTS_PORT) {}
  bool begin();
  void loop() { _ubi.loop(); }
  void addEnv(const EnvData& d);
  bool publish();

 private:
  Ubidots _ubi;
};
```

### ubidots.cpp
Gestiona la conexión WiFi previa a iniciar MQTT, añade las variables etiquetadas definidas en `config.h` y maneja reconexiones en caso de fallos de publicación.
```cpp
#include "ubidots.h"
#include <Arduino.h>
#include <WiFi.h>

bool UbidotsClient::begin() {
  Serial.println("[WIFI] Conectando a red...");
  // Mostrar parámetros de conexión (token enmascarado)
  {
    const char* t = UBIDOTS_TOKEN;
    String masked = String(t);
    if (masked.length() > 6) {
      masked = masked.substring(0, 3) + "..." + masked.substring(masked.length()-3);
    }
    Serial.printf("[UBIDOTS] Host=%s Port=%d TLS=%d Token=%s\n", UBIDOTS_HOST, UBIDOTS_PORT, UBIDOTS_USE_TLS, masked.c_str());
  }
  _ubi.connectToWifi(WIFI_SSID, WIFI_PASS);
  // Esperar a que la WiFi se conecte antes de inicializar MQTT
  unsigned long start = millis();
  const unsigned long timeout = 10000; // 10s
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeout) {
    Serial.print(".");
    delay(200);
  }
  Serial.println();

  bool wifiOk = WiFi.status() == WL_CONNECTED;
  Serial.println(wifiOk ? "[WIFI] Conexion establecida" : "[WIFI] Error al conectar (timeout)");

  if (!wifiOk) {
    return false;
  }

  // Inicializar y conectar el cliente Ubidots ahora que la WiFi está estable
  _ubi.setup();
  _ubi.reconnect();

  return true;
}

void UbidotsClient::addEnv(const EnvData& d) {
  if (d.hasBme) {
    _ubi.add(VAR_TEMP, d.temp);
    _ubi.add(VAR_HUM,  d.hum);
  }
  if (d.hasCcs) {
    _ubi.add(VAR_CO2_PPM, d.eco2);
    _ubi.add(VAR_TVOC_PPB, d.tvoc);
  }
  if (d.hasLight) {
    _ubi.add(VAR_LUX, d.lux);
  }
  if (d.hasNoise) {
    _ubi.add(VAR_NOISE_DB, d.noiseDb);
  }
}

bool UbidotsClient::publish() {
  bool ok = _ubi.publish(DEVICE_LABEL);
  if (ok) {
    Serial.println("[MQTT] Publicado OK");
    return true;
  }

  // Publish failed: intentar diagnosticar la causa
  Serial.println("[MQTT] Fallo al publicar");

  // Estado de la conexión WiFi
  wl_status_t wifiStatus = WiFi.status();
  Serial.printf("[MQTT] WiFi.status() = %d\n", (int)wifiStatus);
  if (wifiStatus != WL_CONNECTED) {
    Serial.println("[MQTT] Motivo: WiFi no conectado");
  }

  // Intentar reconectar al broker Ubidots
  Serial.println("[MQTT] Intentando reconectar al broker...");
  _ubi.reconnect();
  // Comprobar estado de la WiFi tras intentar reconectar
  wl_status_t wifiStatusAfter = WiFi.status();
  Serial.printf("[MQTT] WiFi.status() tras reconexión = %d\n", (int)wifiStatusAfter);
  if (wifiStatusAfter != WL_CONNECTED) {
    Serial.println("[MQTT] Motivo probable: WiFi desconectada tras reconexión");
  } else {
    Serial.println("[MQTT] Se intentó reconectar al broker; si el problema persiste, comprobar credenciales y disponibilidad del broker");
  }

  // Intentar publicar de nuevo tras reconexión
  delay(1000); // dar tiempo a restablecer la conexión
  Serial.println("[MQTT] Intentando publicar de nuevo tras reconexión...");
  bool ok2 = _ubi.publish(DEVICE_LABEL);
  Serial.println(ok2 ? "[MQTT] Publicado OK tras reconexión" : "[MQTT] Sigue fallando la publicación tras reconexión");
  return ok2;
}
```

## Sensor de ruido PDM (`spm1423.h` y `spm1423.cpp`)

### spm1423.h
Define la interfaz del driver del micrófono digital SPM1423, exponiendo métodos para configurar el puerto I2S en modo PDM, capturar muestras y liberar el recurso cuando deja de usarse.
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
Configura el periférico I2S en modo maestro PDM, gestiona el buffer DMA para obtener bloques de audio y convierte las muestras en un nivel RMS expresado en dB SPL, filtrando ruido de fondo antes de reportarlo en `EnvData`.
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
Estructura que centraliza las lecturas de todos los sensores y banderas de disponibilidad, permitiendo que el resto de módulos consuman valores consistentes sin acoplarse a implementaciones concretas.
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
  float eco2  = NAN;
  float tvoc  = NAN;
  float lux   = NAN;
  float noiseDb = NAN;
};
```

### pins.h
Agrupa en constantes los pines asignados a LEDs, buzzer y micrófono PDM para facilitar su reutilización y evitar valores mágicos dispersos por el código.
```cpp
#pragma once
#define LED_VERDE_PIN 13
#define LED_ROJO_PIN  14
#define BUZZER_PIN    25
#define MIC_CLK_PIN   26
#define MIC_DATA_PIN  34
```
