#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "config.h"
#include "env_data.h"

class ThingsboardClient {
 public:
  explicit ThingsboardClient(Client& netClient);

  bool begin();
  void loop();
  void sendEnv(const EnvData& data);

 private:
  Client& _netClient;
  PubSubClient _mqtt;
  unsigned long _lastConnectAttempt = 0;
  bool _lastConnected = false;

  bool ensureConnected();
  String makeClientId();
};
