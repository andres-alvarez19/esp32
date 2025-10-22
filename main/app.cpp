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

  // buzzer will be registered from main.ino after App is initialized

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

  // Nota: las lecturas las hacemos cada ciclo del loop, pero los logs y la
  // publicación se realizan cada 5000 ms para no saturar la consola.


  if (millis() - _lastPublish > 60000) {
    // Log de todos los sensores - imprimir estado y valores leídos (cada 5s)
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
    _oled.draw(_data, 2000.0f);
    _lastPublish = millis();
  }
}
