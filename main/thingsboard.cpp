#include "thingsboard.h"

#include <Arduino.h>
#include <cstring>

#include "buzzer.h"

namespace {
const char* kPrefsNamespace = "wifi";
const char* kPrefsKeySsid = "ssid";
const char* kPrefsKeyPass = "password";
}

ThingsboardClient* ThingsboardClient::_instance = nullptr;

ThingsboardClient::ThingsboardClient(Client& netClient)
    : _netClient(netClient), _mqtt(_netClient) {
  _instance = this;
}

bool ThingsboardClient::begin() {
  Serial.println("[TB] Iniciando cliente ThingsBoard");
  Serial.printf("[TB] Host=%s Port=%d\n", TB_HOST, TB_PORT);

  String ssid = WIFI_SSID;
  String pass = WIFI_PASS;

  bool hadStoredCreds = loadCredentials(ssid, pass);
  if (hadStoredCreds) {
    Serial.printf("[WIFI] Usando SSID guardado: %s\n", ssid.c_str());
  } else {
    Serial.printf("[WIFI] Usando SSID por defecto: %s\n", ssid.c_str());
  }

  bool wifiOk = connectWifi(ssid, pass);
  if (!wifiOk) {
    Serial.println("[WIFI] No se pudo conectar, iniciando portal de configuracion");
    startConfigPortal();
  }

  _mqtt.setServer(TB_HOST, TB_PORT);
  _mqtt.setCallback(ThingsboardClient::mqttCallback);

  return wifiOk;
}

bool ThingsboardClient::loadCredentials(String& ssid, String& pass) {
  Preferences prefs;
  if (!prefs.begin(kPrefsNamespace, true)) {
    Serial.println("[WIFI] No se pudo abrir NVS (lectura)");
    return false;
  }

  String storedSsid = prefs.getString(kPrefsKeySsid, "");
  String storedPass = prefs.getString(kPrefsKeyPass, "");
  prefs.end();

  if (storedSsid.length() == 0) {
    return false;
  }

  ssid = storedSsid;
  pass = storedPass;
  return true;
}

void ThingsboardClient::saveCredentials(const String& ssid, const String& pass) {
  Preferences prefs;
  if (!prefs.begin(kPrefsNamespace, false)) {
    Serial.println("[WIFI] No se pudo abrir NVS para guardar credenciales");
    return;
  }

  prefs.putString(kPrefsKeySsid, ssid);
  prefs.putString(kPrefsKeyPass, pass);
  prefs.end();
  Serial.printf("[WIFI] Credenciales guardadas SSID=%s\n", ssid.c_str());
}

bool ThingsboardClient::connectWifi(const String& ssid, const String& pass) {
  if (ssid.length() == 0) {
    Serial.println("[WIFI] SSID vacio, no se puede conectar");
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long start = millis();
  Serial.printf("[WIFI] Conectando a SSID=%s\n", ssid.c_str());
  while (WiFi.status() != WL_CONNECTED &&
         (millis() - start) < WIFI_CONNECT_TIMEOUT_MS) {
    Serial.print(".");
    delay(200);
  }
  Serial.println();

  bool ok = WiFi.status() == WL_CONNECTED;
  if (ok) {
    _wifiConnected = true;
    IPAddress ip = WiFi.localIP();
    Serial.printf("[WIFI] Conectado. IP=%s\n", ipToString(ip).c_str());
    _statusMessage = "WiFi OK";
    updateOledIp(ip, "IP");
  } else {
    Serial.println("[WIFI] Error al conectar WiFi (timeout)");
    _wifiConnected = false;
  }

  return ok;
}

void ThingsboardClient::startConfigPortal() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  bool apOk = WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
  _apIp = WiFi.softAPIP();
  _apMode = true;
  _wifiConnected = false;

  Serial.printf("[WIFI] AP config '%s' %s IP=%s\n", WIFI_AP_SSID,
                apOk ? "iniciado" : "NO iniciado", ipToString(_apIp).c_str());

  _server.on("/", HTTP_GET, [this]() { handleRoot(); });
  _server.on("/save", HTTP_POST, [this]() { handleSave(); });
  _server.begin();

  _statusMessage = "Config WiFi";
  updateOledIp(_apIp, "AP IP");
}

void ThingsboardClient::handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Configurar WiFi ESP32</title>
  <style>
    body { font-family: Arial, sans-serif; max-width: 420px; margin: 30px auto; }
    label { display: block; margin-top: 12px; }
    input { width: 100%; padding: 8px; margin-top: 6px; box-sizing: border-box; }
    button { margin-top: 16px; padding: 10px 14px; width: 100%; }
  </style>
</head>
<body>
  <h2>Configuracion WiFi</h2>
  <form action="/save" method="POST">
    <label>SSID
      <input type="text" name="ssid" required>
    </label>
    <label>Password
      <input type="password" name="password" required>
    </label>
    <button type="submit">Guardar y reiniciar</button>
  </form>
</body>
</html>
)rawliteral";
  _server.send(200, "text/html", html);
}

void ThingsboardClient::handleSave() {
  if (!_server.hasArg("ssid") || !_server.hasArg("password")) {
    _server.send(400, "text/plain", "Faltan parametros");
    return;
  }

  String ssid = _server.arg("ssid");
  String pass = _server.arg("password");

  // Limpiar credenciales previas antes de guardar las nuevas
  clearCredentials();
  saveCredentials(ssid, pass);
  _server.send(200, "text/plain",
               "Credenciales guardadas. El dispositivo se reiniciara...");
  _shouldReboot = true;
}

void ThingsboardClient::clearCredentials() {
  Preferences prefs;
  if (!prefs.begin(kPrefsNamespace, false)) {
    Serial.println("[WIFI] No se pudo abrir NVS para limpiar credenciales");
    return;
  }
  prefs.clear();
  prefs.end();
  Serial.println("[WIFI] Credenciales previas eliminadas");
}

String ThingsboardClient::makeClientId() {
  String cid = "esp32-";
  uint64_t chipId = ESP.getEfuseMac();
  cid += String((uint32_t)(chipId >> 32), HEX);
  cid += String((uint32_t)chipId, HEX);
  return cid;
}

bool ThingsboardClient::ensureConnected() {
  if (_apMode) {
    return false;
  }

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
  if (_apMode) {
    _server.handleClient();
    if (_shouldReboot) {
      delay(1200);
      ESP.restart();
    }
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    if (_wifiConnected) {
      Serial.println("[TB] WiFi desconectado en loop()");
    }
    _wifiConnected = false;
    _statusMessage = "WiFi OFF";
    if (!_rpcIndicators) {
      _oledLine1 = "WiFi OFF";
      _oledLine2 = "";
    }
    return;
  }

  if (!_wifiConnected) {
    _wifiConnected = true;
    IPAddress ip = WiFi.localIP();
    Serial.printf("[TB] WiFi reconectado IP=%s\n", ipToString(ip).c_str());
    _statusMessage = "WiFi OK";
    updateOledIp(ip, "IP");
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

  if (_buzzer != nullptr && _buzzerOffDeadline != 0) {
    if ((long)(millis() - _buzzerOffDeadline) >= 0) {
      _buzzer->off();
      _buzzerOffDeadline = 0;
    }
  }
}

void ThingsboardClient::sendEnv(const EnvData& data) {
  if (_apMode) {
    Serial.println("[TB] Modo AP de configuracion, no se publica telemetria");
    return;
  }

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
  const char* line1 = params["oledLine1"] | "";
  const char* line2 = params["oledLine2"] | "";

  Serial.printf("[TB] RPC setIndicators: ledGreen=%d ledRed=%d buzzerMs=%lu status='%s' line1='%s' line2='%s'\n",
                ledGreenOn, ledRedOn, buzzerMs, statusMsg, line1, line2);
  _statusMessage = statusMsg;
  _oledLine1 = line1;
  _oledLine2 = line2;

  // Sonar buzzer solo si cambian LEDs o texto
  bool stateChanged = false;
  if (!_hasLastRpcState) {
    stateChanged = true;
    _hasLastRpcState = true;
  } else {
    bool oledChanged = (_oledLine1 != _lastLine1) || (_oledLine2 != _lastLine2);
    bool ledsChanged = (ledGreenOn != _lastLedGreen) || (ledRedOn != _lastLedRed);
    stateChanged = oledChanged || ledsChanged;
  }

  unsigned long buzzerForApply = stateChanged ? buzzerMs : 0;
  applyIndicators(ledGreenOn, ledRedOn, buzzerForApply);
  _rpcIndicators = true;

  bool telemFromRpc = false;
  // Actualizar valores para OLED si vienen en el payload RPC
  if (!params["bme_temp_c"].isNull()) { _oledTempC = params["bme_temp_c"].as<float>(); telemFromRpc = true; }
  if (!params["bme_hum_pct"].isNull()) { _oledHumPct = params["bme_hum_pct"].as<float>(); telemFromRpc = true; }
  if (!params["sgp30_eco2_ppm"].isNull()) { _oledEco2Ppm = params["sgp30_eco2_ppm"].as<float>(); telemFromRpc = true; }
  if (!params["sgp30_tvoc_ppb"].isNull()) { _oledTvocPpb = params["sgp30_tvoc_ppb"].as<float>(); telemFromRpc = true; }
  if (!params["bh1750_lux"].isNull()) { _oledLux = params["bh1750_lux"].as<float>(); telemFromRpc = true; }
  if (!params["spm1423_noise_db"].isNull()) { _oledNoiseDb = params["spm1423_noise_db"].as<float>(); telemFromRpc = true; }

  if (telemFromRpc) {
    Serial.printf("[TB] RPC telemetria->OLED T=%.1fC H=%.1f%% eCO2=%.0fppm TVOC=%.0fppb Lux=%.0f Noise=%.1fdB\n",
                  _oledTempC, _oledHumPct, _oledEco2Ppm, _oledTvocPpb, _oledLux, _oledNoiseDb);
  }

  // Guardar estado previo para la próxima comparación
  _lastLine1 = _oledLine1;
  _lastLine2 = _oledLine2;
  _lastLedGreen = ledGreenOn;
  _lastLedRed = ledRedOn;
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

void ThingsboardClient::updateOledIp(const IPAddress& ip, const char* label) {
  if (_rpcIndicators) return;
  _oledLine1 = label;
  _oledLine2 = ipToString(ip);
}

String ThingsboardClient::ipToString(const IPAddress& ip) const {
  return ip.toString();
}
