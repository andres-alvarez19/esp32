#pragma once
#include <UbidotsEsp32Mqtt.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"
#include "env_data.h"
#include "buzzer.h"

class UbidotsClient {
 public:
  explicit UbidotsClient(const char* token = UBIDOTS_TOKEN)
  : _ubi(token, UBIDOTS_HOST, UBIDOTS_PORT), _mqtt(_wifiClient) {}
  bool begin();
  void loop();
  void addEnv(const EnvData& d);
  bool publish();
  bool publishUbi(const char* deviceLabel, const String& json);
  String makeClientId();
  // Register buzzer to allow callbacks to control it
  void registerBuzzer(ActiveBuzzer* buzzer) { _buzzer = buzzer; }

 private:
  Ubidots _ubi;
  WiFiClient _wifiClient;
  PubSubClient _mqtt;
  String _lastPayloadJson;
  ActiveBuzzer* _buzzer = nullptr;
  bool _buzzerState = false;
};
