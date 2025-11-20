#pragma once
#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "env_data.h"
#include "bme280.h"
#include "sgp30.h"
#include "bh1750.h"
#include "spm1423.h"
#include "oled.h"
#include "thingsboard.h"

// Declaración adelantada para poder usar el puntero sin incluir buzzer.h aquí
class ActiveBuzzer;

class App {
 public:
  App() : _tb(_wifiClient) {}
  bool begin();
  void update();
  void registerBuzzer(ActiveBuzzer* /*buzzer*/) { /* reservado para futuras callbacks MQTT */ }
  bool modulesReady() const { return _modulesReady; }
  const EnvData& data() const { return _data; }
  const String& failedModules() const { return _failedModules; }

 private:
  BME280Sensor _bme;
  SGP30Sensor  _sgp;
  BH1750Sensor _light;
  SPM1423Sensor _sound;
  OledView      _oled;
  WiFiClient      _wifiClient;
  ThingsboardClient _tb;
  EnvData _data;

  unsigned long _lastPublish = 0;
  bool _modulesReady = false;
  String _failedModules;
};
