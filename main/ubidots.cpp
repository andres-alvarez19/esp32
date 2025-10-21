#include "ubidots.h"
#include <Arduino.h>
#include <WiFi.h>

bool UbidotsClient::begin() {
  Serial.println("[WIFI] Conectando a red...");
  _ubi.connectToWifi(WIFI_SSID, WIFI_PASS);
  _ubi.setup();
  _ubi.reconnect();

  bool wifiOk = WiFi.status() == WL_CONNECTED;
  Serial.println(wifiOk ? "[WIFI] Conexion establecida" : "[WIFI] Error al conectar");
  return wifiOk;
}

void UbidotsClient::addEnv(const EnvData& d) {
  if (d.hasBme) {
    _ubi.add(VAR_TEMP, d.temp);
    _ubi.add(VAR_HUM,  d.hum);
    _ubi.add(VAR_PRESS, d.press);
    _ubi.add(VAR_ALT, d.alt);
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
  Serial.println(ok ? "[MQTT] Publicado OK" : "[MQTT] Fallo al publicar");
  return ok;
}
