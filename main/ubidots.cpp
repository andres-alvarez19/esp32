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
  int added = 0;
  auto addIf = [&](const char* name, float v) {
    if (!isfinite(v)) {
      Serial.printf("[UBI] Skipping %s => NaN\n", name);
      return;
    }
    Serial.printf("[UBI] Adding %s => %.2f\n", name, v);
    _ubi.add(name, v);
    ++added;
  };

  if (d.hasBme) {
    addIf(VAR_TEMP, d.temp);
    addIf(VAR_HUM, d.hum);
    addIf(VAR_PRESS, d.press);
    addIf(VAR_ALT, d.alt);
  } else {
    Serial.println("[UBI] BME not present, skipping temperature/humidity/pressure/altitude");
  }

  if (d.hasCcs) {
    addIf(VAR_CO2_PPM, d.eco2);
    addIf(VAR_TVOC_PPB, d.tvoc);
  } else {
    Serial.println("[UBI] SGP30 not present, skipping eCO2/TVOC");
  }

  if (d.hasLight) {
    addIf(VAR_LUX, d.lux);
  } else {
    Serial.println("[UBI] BH1750 not present, skipping lux");
  }

  if (d.hasNoise) {
    addIf(VAR_NOISE_DB, d.noiseDb);
  } else {
    Serial.println("[UBI] Microfono no presente, skipping noise dB");
  }

  Serial.printf("[UBI] Total variables añadidas al payload: %d\n", added);
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
