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
    _ubi.addEnv(_data);
    _ubi.publish();
    _oled.draw(_data, 2000.0f);
    _lastPublish = millis();
  }
}
