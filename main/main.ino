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

  // Register global buzzer with the app so MQTT callbacks can control it
  app.registerBuzzer(&buzzer);

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
