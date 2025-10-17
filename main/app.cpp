#include "app.h"
#include <Arduino.h>

void App::begin() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== Proyecto ESP32 – Monitor Ambiental ===");

  _bme.begin();
  _sgp.begin();
  _oled.begin();
  _ubi.begin();

  pinMode(LED_VERDE_PIN, OUTPUT);
  pinMode(LED_ROJO_PIN, OUTPUT);
  digitalWrite(LED_VERDE_PIN, HIGH);
  digitalWrite(LED_ROJO_PIN, HIGH);

  _lastPublish = millis();
}

void App::loop() {
  _ubi.loop();

  // Leer sensores
  _bme.read(_data);
  _sgp.read(_data, _data.temp, _data.hum);

  // Control visual según CO2
  if (_data.eco2 > 1500) {
    digitalWrite(LED_ROJO_PIN, HIGH);
    digitalWrite(LED_VERDE_PIN, LOW);
  } else {
    digitalWrite(LED_ROJO_PIN, LOW);
    digitalWrite(LED_VERDE_PIN, HIGH);
  }

  // Publicar y actualizar display cada 5 s
  if (millis() - _lastPublish > 5000) {
    _ubi.addEnv(_data);
    _ubi.publish();
    _oled.draw(_data, 2000.0f);
    _lastPublish = millis();
  }
}
