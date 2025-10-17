#pragma once
#include <UbidotsEsp32Mqtt.h>
#include "config.h"
#include "env_data.h"

class UbidotsClient {
 public:
  explicit UbidotsClient(const char* token = UBIDOTS_TOKEN)
  : _ubi(token, "stem.ubidots.com", 1883) {}
  void begin();
  void loop() { _ubi.loop(); }
  void addEnv(const EnvData& d);
  bool publish();

 private:
  Ubidots _ubi;
};
