#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "config.h"
#include "env_data.h"

class ActiveBuzzer;

class ThingsboardClient {
 public:
  explicit ThingsboardClient(Client& netClient);

  bool begin();
  void loop();
  void sendEnv(const EnvData& data);
  void setIndicatorsHardware(uint8_t ledGreenPin, uint8_t ledRedPin, ActiveBuzzer* buzzer);
  bool indicatorsManagedByRpc() const { return _rpcIndicators; }
   const String& statusMessage() const { return _statusMessage; }

 private:
  static ThingsboardClient* _instance;

  Client& _netClient;
  PubSubClient _mqtt;
  unsigned long _lastConnectAttempt = 0;
  bool _lastConnected = false;
  uint8_t _ledGreenPin = 0xFF;
  uint8_t _ledRedPin = 0xFF;
  ActiveBuzzer* _buzzer = nullptr;
  bool _rpcIndicators = false;
  unsigned long _buzzerOffDeadline = 0;
  String _statusMessage = "OK";

  static void mqttCallback(char* topic, uint8_t* payload, unsigned int length);
  void handleRpcMessage(const char* topic, const uint8_t* payload, unsigned int length);
  void applyIndicators(bool ledGreenOn, bool ledRedOn, unsigned long buzzerMs);
  bool ensureConnected();
  String makeClientId();
};
