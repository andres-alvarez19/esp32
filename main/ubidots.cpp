#include "ubidots.h"
#include <Arduino.h>

void UbidotsClient::begin() {
  _ubi.connectToWifi(WIFI_SSID, WIFI_PASS);
  _ubi.setup();
  _ubi.reconnect();
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
}

bool UbidotsClient::publish() {
  bool ok = _ubi.publish(DEVICE_LABEL);
  Serial.println(ok ? "[MQTT] Publicado OK" : "[MQTT] Fallo al publicar");
  return ok;
}
