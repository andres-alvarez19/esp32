#include "thingsboard.h"

#include <Arduino.h>
#include <cstring>

#include "buzzer.h"

ThingsboardClient* ThingsboardClient::_instance = nullptr;

ThingsboardClient::ThingsboardClient(Client& netClient)
    : _netClient(netClient), _mqtt(_netClient) {
  _instance = this;
}

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
  _mqtt.setCallback(ThingsboardClient::mqttCallback);

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

  if (connected) {
    const char* rpcTopic = "v1/devices/me/rpc/request/+";
    bool subOk = _mqtt.subscribe(rpcTopic);
    Serial.printf("[TB] Subcripcion RPC (%s): %s\n", rpcTopic,
                  subOk ? "OK" : "FALLO");
  }

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

  // Apagar buzzer cuando expira la ventana definida por RPC, sin depender de MQTT
  if (_buzzer != nullptr && _buzzerOffDeadline != 0) {
    if ((long)(millis() - _buzzerOffDeadline) >= 0) {
      _buzzer->off();
      _buzzerOffDeadline = 0;
    }
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

void ThingsboardClient::setIndicatorsHardware(uint8_t ledGreenPin, uint8_t ledRedPin,
                                              ActiveBuzzer* buzzer) {
  _ledGreenPin = ledGreenPin;
  _ledRedPin = ledRedPin;
  _buzzer = buzzer;
}

void ThingsboardClient::mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
  Serial.printf("[TB] Payload recibido topic=%s len=%u\n", topic ? topic : "(null)", length);
  if (_instance != nullptr) {
    _instance->handleRpcMessage(topic, payload, length);
  }
}

void ThingsboardClient::handleRpcMessage(const char* topic, const uint8_t* payload,
                                         unsigned int length) {
  const char* rpcPrefix = "v1/devices/me/rpc/request/";
  if (strncmp(topic, rpcPrefix, strlen(rpcPrefix)) != 0) {
    return;
  }

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, payload, length);
  if (err) {
    Serial.printf("[TB] Error al parsear RPC: %s\n", err.c_str());
    return;
  }

  const char* method = doc["method"];
  if (method == nullptr) {
    Serial.println("[TB] RPC sin 'method'");
    return;
  }

  if (strcmp(method, "setIndicators") != 0) {
    Serial.printf("[TB] RPC '%s' ignorado\n", method);
    return;
  }

  JsonObject params = doc["params"];
  if (params.isNull()) {
    Serial.println("[TB] RPC setIndicators sin params");
    return;
  }

  bool ledGreenOn = params["ledGreen"] | false;
  bool ledRedOn = params["ledRed"] | false;
  unsigned long buzzerMs = params["buzzerMs"] | 0;
  const char* statusMsg = params["statusMessage"] | "OK";

  Serial.printf("[TB] RPC setIndicators: ledGreen=%d ledRed=%d buzzerMs=%lu\n",
                ledGreenOn, ledRedOn, buzzerMs);
  _statusMessage = statusMsg;

  applyIndicators(ledGreenOn, ledRedOn, buzzerMs);
  _rpcIndicators = true;
}

void ThingsboardClient::applyIndicators(bool ledGreenOn, bool ledRedOn,
                                        unsigned long buzzerMs) {
  if (_ledGreenPin != 0xFF) {
    digitalWrite(_ledGreenPin, ledGreenOn ? HIGH : LOW);
  }
  if (_ledRedPin != 0xFF) {
    digitalWrite(_ledRedPin, ledRedOn ? HIGH : LOW);
  }

  if (_buzzer != nullptr && _buzzer->isReady()) {
    if (buzzerMs > 0) {
      _buzzer->on();
      _buzzerOffDeadline = millis() + buzzerMs;
    } else {
      _buzzer->off();
      _buzzerOffDeadline = 0;
    }
  }
}
