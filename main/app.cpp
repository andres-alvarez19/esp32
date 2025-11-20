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
  Serial.println("[APP] Iniciando OLED...");
  checkModule("OLED", _oled.begin());
  Serial.println("[APP] Iniciando ThingsBoard (WiFi+MQTT)...");
  checkModule("ThingsBoard", _tb.begin());

  // buzzer will be registered from main.ino after App is initialized

  _modulesReady = modulesOk;
  if (!_modulesReady) {
    Serial.print("[APP] Modulos con fallo: ");
    Serial.println(_failedModules);
    // Forzar primer ciclo de envio para ver datos aunque falten modulos
    _lastPublish = millis() - 10000;
  } else {
    // Armar primer envio pronto tras el arranque
    _lastPublish = millis();
    Serial.println("[APP] Inicio completado, listo para publicar");
  }

  return _modulesReady;
}

void App::update() {
  _tb.loop();

  if (!_modulesReady) {
    static bool warned = false;
    if (!warned) {
      Serial.printf("[APP] Ejecutando con modulos fallidos: %s\n", _failedModules.c_str());
      warned = true;
    }
  }

  _bme.read(_data);
  _sgp.read(_data, _data.temp, _data.hum);
  _light.read(_data);
  _sound.read(_data);

  // Logs y envío cada "kPublishIntervalMs"
  constexpr unsigned long kPublishIntervalMs = 10000; // 10 s
  if (millis() - _lastPublish < kPublishIntervalMs) return;

  Serial.println("\n[APP] ----- Ciclo de sensores y envio -----");
  Serial.println("[APP] Preparando lectura de sensores (se muestra por modulo):");

  auto fmt = [](float v) -> String {
    if (isnan(v)) return String("N/A");
    char buf[16];
    snprintf(buf, sizeof(buf), "%.2f", v);
    return String(buf);
  };

  auto printFloat = [](const char* name, float v) {
    if (isnan(v)) {
      Serial.printf("%s: N/A\n", name);
    } else {
      Serial.printf("%s: %.2f\n", name, v);
    }
  };

  Serial.println("[APP] -> BME280");
  if (_data.hasBme) {
    printFloat("  Temp (C)", _data.temp);
    printFloat("  Hum (%)", _data.hum);
  } else {
    Serial.println("  No detectado");
  }

  Serial.println("[APP] -> SGP30");
  if (_data.hasCcs) {
    printFloat("  eCO2 (ppm)", _data.eco2);
    printFloat("  TVOC (ppb)", _data.tvoc);
  } else {
    Serial.println("  No detectado");
  }

  Serial.println("[APP] -> BH1750");
  if (_data.hasLight) {
    printFloat("  Lux", _data.lux);
  } else {
    Serial.println("  No detectado");
  }

  Serial.println("[APP] -> SPM1423");
  if (_data.hasNoise) {
    printFloat("  Noise dB SPL", _data.noiseDb);
  } else {
    Serial.println("  No detectado");
  }

  Serial.println("[APP] Enviando telemetria a ThingsBoard...");
  Serial.printf("[APP] Resumen: Temp=%s Hum=%s eCO2=%s TVOC=%s Lux=%s Noise=%s\n",
                fmt(_data.temp).c_str(), fmt(_data.hum).c_str(),
                fmt(_data.eco2).c_str(), fmt(_data.tvoc).c_str(),
                fmt(_data.lux).c_str(), fmt(_data.noiseDb).c_str());
  _tb.sendEnv(_data);

  _oled.draw(_data, 2000.0f);
  _lastPublish = millis();
}
