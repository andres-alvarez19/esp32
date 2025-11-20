#include "thingsboard.h"

#include <Arduino.h>

ThingsboardClient::ThingsboardClient(Client& netClient)
    : _netClient(netClient), _mqtt(_netClient) {}

bool ThingsboardClient::begin() {
  Serial.println("[TB] Iniciando cliente ThingsBoard");

  Serial.printf("[TB] Host=%s Port=%d\n", TB_HOST, TB_PORT);

  Serial.printf("[WIFI] Conectando a SSID=%s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  const unsigned long timeout = 10000;  // 10s
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeout) {
    Serial.print(".");
    delay(200);
  }
  Serial.println();

  bool wifiOk = WiFi.status() == WL_CONNECTED;
  Serial.println(wifiOk ? "[WIFI] Conexion WiFi establecida"
                        : "[WIFI] Error al conectar WiFi (timeout)");

  _mqtt.setServer(TB_HOST, TB_PORT);

  return wifiOk;
}

String ThingsboardClient::makeClientId() {
  String cid = "esp32-";
  uint64_t chipId = ESP.getEfuseMac();
  cid += String((uint32_t)(chipId >> 32), HEX);
  cid += String((uint32_t)chipId, HEX);
  return cid;
}

bool ThingsboardClient::ensureConnected() {
  if (_mqtt.connected()) {
    if (!_lastConnected) {
      _lastConnected = true;
    }
    return true;
  }

  unsigned long now = millis();
  const unsigned long kReconnectInterval = 5000;  // 5s
  if (now - _lastConnectAttempt < kReconnectInterval) {
    return false;
  }
  _lastConnectAttempt = now;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[TB] WiFi no conectado, no se puede conectar MQTT");
    return false;
  }

  String clientId = makeClientId();
  Serial.printf("[TB] Conectando a %s:%d\n", TB_HOST, TB_PORT);

  bool connected =
      _mqtt.connect(clientId.c_str(), TB_ACCESS_TOKEN, /*pass*/ nullptr);
  Serial.printf("[TB] Conexion MQTT %s\n", connected ? "OK" : "FALLO");

  _lastConnected = connected;

  return connected;
}

void ThingsboardClient::loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[TB] WiFi no conectado en loop(), se omite MQTT");
    return;
  }

  if (!_mqtt.connected()) {
    ensureConnected();
  }

  if (_mqtt.connected()) {
    if (!_lastConnected) {
      _lastConnected = true;
    }
    _mqtt.loop();
  } else if (_lastConnected) {
    Serial.println("[TB] MQTT desconectado");
    _lastConnected = false;
  }
}

void ThingsboardClient::sendEnv(const EnvData& data) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[TB] WiFi no conectado, no se publica telemetria");
    return;
  }

  if (!ensureConnected()) {
    Serial.println("[TB] MQTT no conectado, no se publica telemetria");
    return;
  }

  StaticJsonDocument<256> doc;
  int added = 0;

  auto addIf = [&](const char* key, float value) {
    if (!isfinite(value)) return;
    doc[key] = value;
    ++added;
  };

  if (data.hasBme) {
    addIf(VAR_TEMP_C, data.temp);
    addIf(VAR_HUM_PCT, data.hum);
  }

  if (data.hasCcs) {
    addIf(VAR_ECO2_PPM, data.eco2);
    addIf(VAR_TVOC_PPB, data.tvoc);
  }

  if (data.hasLight) {
    addIf(VAR_LUX, data.lux);
  }

  if (data.hasNoise) {
    addIf(VAR_NOISE_DB, data.noiseDb);
  }

  if (added == 0) {
    Serial.println("[TB] No hay datos validos para enviar");
    return;
  }

  String payload;
  serializeJson(doc, payload);

  const char* topic = "v1/devices/me/telemetry";
  bool ok = _mqtt.publish(topic, payload.c_str());
  Serial.printf("[TB] Publicacion %s\n", ok ? "OK" : "FALLO");
}
