#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <Preferences.h>
#include <math.h>

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
  float oledTempC() const { return _oledTempC; }
  float oledHumPct() const { return _oledHumPct; }
  float oledEco2Ppm() const { return _oledEco2Ppm; }
  float oledTvocPpb() const { return _oledTvocPpb; }
  float oledLux() const { return _oledLux; }
  float oledNoiseDb() const { return _oledNoiseDb; }
  const String& oledLine1() const { return _oledLine1; }
  const String& oledLine2() const { return _oledLine2; }

 private:
  bool loadCredentials(String& ssid, String& pass);
  void saveCredentials(const String& ssid, const String& pass);
  bool connectWifi(const String& ssid, const String& pass);
  void startConfigPortal();
  void handleRoot();
  void handleSave();
  void clearCredentials();
  void updateOledIp(const IPAddress& ip, const char* label);
  String ipToString(const IPAddress& ip) const;

  static ThingsboardClient* _instance;

  Client& _netClient;
  PubSubClient _mqtt;
  WebServer _server{80};
  unsigned long _lastConnectAttempt = 0;
  bool _lastConnected = false;
  bool _wifiConnected = false;
  bool _apMode = false;
  bool _shouldReboot = false;
  IPAddress _apIp;
  uint8_t _ledGreenPin = 0xFF;
  uint8_t _ledRedPin = 0xFF;
  ActiveBuzzer* _buzzer = nullptr;
  bool _rpcIndicators = false;
  unsigned long _buzzerOffDeadline = 0;
  String _statusMessage = "OK";
  float _oledTempC = NAN;
  float _oledHumPct = NAN;
  float _oledEco2Ppm = NAN;
  float _oledTvocPpb = NAN;
  float _oledLux = NAN;
  float _oledNoiseDb = NAN;
  String _oledLine1;
  String _oledLine2;
  String _lastLine1;
  String _lastLine2;
  bool _lastLedGreen = false;
  bool _lastLedRed = false;
  bool _hasLastRpcState = false;

  static void mqttCallback(char* topic, uint8_t* payload, unsigned int length);
  void handleRpcMessage(const char* topic, const uint8_t* payload, unsigned int length);
  void applyIndicators(bool ledGreenOn, bool ledRedOn, unsigned long buzzerMs);
  bool ensureConnected();
  String makeClientId();
};
