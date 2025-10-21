#include "app.h"
#include <Arduino.h>

void App::begin() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== Proyecto ESP32 – Monitor Ambiental ===");

  pinMode(LED_VERDE_PIN, OUTPUT);
  pinMode(LED_ROJO_PIN, OUTPUT);
  digitalWrite(LED_VERDE_PIN, LOW);
  digitalWrite(LED_ROJO_PIN, LOW);

  bool modulesOk = true;
  String failedModules;
  auto checkModule = [&](const char* name, bool ok) {
    if (ok) {
      Serial.printf("[APP] %s inicializado\n", name);
    } else {
      if (!failedModules.isEmpty()) failedModules += ", ";
      failedModules += name;
      Serial.printf("[APP] Error al iniciar %s\n", name);
      modulesOk = false;
    }
  };

  checkModule("Buzzer", _buzzer.begin());
  checkModule("BME280", _bme.begin());
  checkModule("SGP30", _sgp.begin());
  checkModule("BH1750", _light.begin());
  checkModule("SPM1423", _sound.begin());
  checkModule("OLED", _oled.begin());
  checkModule("Ubidots", _ubi.begin());

  _modulesReady = modulesOk;
  if (!_modulesReady) {
    Serial.print("[APP] Modulos con fallo: ");
    Serial.println(failedModules);
    digitalWrite(LED_ROJO_PIN, HIGH);
    digitalWrite(LED_VERDE_PIN, LOW);
    _buzzer.off();
    return;
  }

  digitalWrite(LED_VERDE_PIN, HIGH);
  digitalWrite(LED_ROJO_PIN, LOW);
  _buzzer.off();
  _lastPublish = millis();
}

void App::loop() {
  _ubi.loop();

  if (!_modulesReady) {
    digitalWrite(LED_ROJO_PIN, HIGH);
    digitalWrite(LED_VERDE_PIN, LOW);
    _buzzer.off();
    return;
  }

  // Leer sensores
  _bme.read(_data);
  _sgp.read(_data, _data.temp, _data.hum);
  _light.read(_data);
  _sound.read(_data);

  // Control visual según CO2
  if (_data.eco2 > 1500) {
    digitalWrite(LED_ROJO_PIN, HIGH);
    digitalWrite(LED_VERDE_PIN, LOW);
    _buzzer.on();
  } else {
    digitalWrite(LED_ROJO_PIN, LOW);
    digitalWrite(LED_VERDE_PIN, HIGH);
    _buzzer.off();
  }

  // Publicar y actualizar display cada 5 s
  if (millis() - _lastPublish > 5000) {
    _ubi.addEnv(_data);
    _ubi.publish();
    _oled.draw(_data, 2000.0f);
    _lastPublish = millis();
  }
}
