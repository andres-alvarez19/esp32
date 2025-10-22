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

  // Configurar cliente MQTT (PubSubClient) sobre WiFiClient
  _mqtt.setServer(UBIDOTS_HOST, UBIDOTS_PORT);

  return true;
}

void UbidotsClient::addEnv(const EnvData& d) {
  int added = 0;
  // Construir JSON manualmente para poder mostrarlo antes de enviarlo
  String payload = "{";
  auto addIf = [&](const char* name, float v) {
    if (!isfinite(v)) {
      Serial.printf("[UBI] Skipping %s => NaN\n", name);
      return;
    }
    Serial.printf("[UBI] Adding %s => %.2f\n", name, v);
    if (added > 0) payload += ",";
    payload += "\"";
    payload += name;
    payload += "\":";
    // Agregar como número simple (Ubidots acepta {"var": 123} o {"var": {"value":123}})
    payload += String(v, 2);
    // También añadir a la estructura interna de la librería para compatibilidad
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

  payload += "}";
  _lastPayloadJson = payload;
  Serial.printf("[UBI] Total variables añadidas al payload: %d\n", added);
  Serial.printf("[UBI] Payload JSON pre-envío: %s\n", _lastPayloadJson.c_str());
  // Si el payload es grande, podemos dividirlo en dos publicaciones: ambientales y gas/ruido
  const size_t kMaxSingle = 512;
  if (_lastPayloadJson.length() > kMaxSingle) {
    Serial.println("[UBI] Payload grande, intentando dividir en dos publicaciones");
    // Construir dos payloads simples: env (BME + lux) y gas/noise (SGP30 + noise)
    String envPayload = "{";
    String gasPayload = "{";
    int envCount = 0;
    int gasCount = 0;
    auto splitAdd = [&](const char* name, float v, bool toEnv) {
      if (!isfinite(v)) return;
      if (toEnv) {
        if (envCount > 0) envPayload += ",";
        envPayload += "\"" + String(name) + "\":" + String(v, 2);
        ++envCount;
      } else {
        if (gasCount > 0) gasPayload += ",";
        gasPayload += "\"" + String(name) + "\":" + String(v, 2);
        ++gasCount;
      }
    };
    if (d.hasBme) {
      splitAdd(VAR_TEMP, d.temp, true);
      splitAdd(VAR_HUM, d.hum, true);
      splitAdd(VAR_PRESS, d.press, true);
      splitAdd(VAR_ALT, d.alt, true);
    }
    if (d.hasCcs) {
      splitAdd(VAR_CO2_PPM, d.eco2, false);
      splitAdd(VAR_TVOC_PPB, d.tvoc, false);
    }
    if (d.hasLight) {
      splitAdd(VAR_LUX, d.lux, true);
    }
    if (d.hasNoise) {
      splitAdd(VAR_NOISE_DB, d.noiseDb, false);
    }
    envPayload += "}";
    gasPayload += "}";
    if (envCount > 0) publishUbi(DEVICE_LABEL, envPayload);
    if (gasCount > 0) publishUbi(DEVICE_LABEL, gasPayload);
  } else {
    // Publicar el JSON completo
    publishUbi(DEVICE_LABEL, _lastPayloadJson);
  }
}

String UbidotsClient::makeClientId() {
  String base = DEVICE_LABEL;
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  uint32_t r = esp_random();
  String rnd = String(r, HEX);
  // Asegurar al menos 20 caracteres: base + '-' + mac + rnd
  return base + "-" + mac + rnd;
}

bool UbidotsClient::publishUbi(const char* deviceLabel, const String& json) {
  String topic = String("/v1.6/devices/") + deviceLabel;
  Serial.printf("[MQTT] Publicando en topic %s payload=%s\n", topic.c_str(), json.c_str());
  // Asegurarse de estar conectado
  if (!_mqtt.connected()) {
    Serial.println("[MQTT] No conectado, intentando conectar...");
    bool connected = _mqtt.connect(makeClientId().c_str(), UBIDOTS_TOKEN, "");
    Serial.printf("[MQTT] Conectado? %d\n", connected);
    if (!connected) {
      // volver a intentar con la librería Ubidots (fallback)
      _ubi.reconnect();
    }
  }
  bool ok = false;
  if (_mqtt.connected()) {
    // PubSubClient publish devuelve bool, QoS depende de la librería; PubSubClient no soporta QoS1 en publish simple
    ok = _mqtt.publish(topic.c_str(), json.c_str());
  } else {
    // fallback a la librería Ubidots (que ya tiene publish)
    Serial.println("[MQTT] Usando fallback _ubi.publish() (librería Ubidots)");
    ok = _ubi.publish(deviceLabel);
  }

  if (!ok) {
    Serial.println("[MQTT] Publicación fallida, intento reconectar y reintentar una vez");
    if (!_mqtt.connected()) {
      bool rc = _mqtt.connect(makeClientId().c_str(), UBIDOTS_TOKEN, "");
      Serial.printf("[MQTT] Reconnected? %d\n", rc);
    }
    if (_mqtt.connected()) {
      ok = _mqtt.publish(topic.c_str(), json.c_str());
      Serial.printf("[MQTT] Reintento -> %d\n", ok);
    }
  }
  return ok;
}

bool UbidotsClient::publish() {
  Serial.printf("[MQTT] Publicando payload: %s\n", _lastPayloadJson.c_str());
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
