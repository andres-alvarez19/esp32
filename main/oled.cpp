#include "oled.h"
#include <Arduino.h>
#include <math.h>

bool OledView::begin(uint8_t addr) {
  // Verificar si responde en el bus I2C antes de inicializar la libreria
  Wire.beginTransmission(addr);
  uint8_t err = Wire.endTransmission();
  if (err != 0) {
    Serial.printf("[OLED] No detectado en 0x%02X (err=%u)\n", addr, err);
    _ok = false;
    return false;
  }

  _ok = _disp.begin(SSD1306_SWITCHCAPVCC, addr);
  if (_ok) {
    _disp.clearDisplay();
    _disp.setTextColor(SSD1306_WHITE);
    _disp.setTextSize(1);
    _disp.setCursor(0, 0);
    _disp.println("OLED OK");
    _disp.display();
  } else {
    Serial.println("[OLED] No detectado");
  }
  return _ok;
}

void OledView::drawRemote(const String& status, const String& line1, const String& line2,
                          float tempC, float humPct, float eco2Ppm, float tvocPpb,
                          float lux, float noiseDb) {
  if (!_ok) return;
  _disp.clearDisplay();

  bool isOk = status.equalsIgnoreCase("OK");
  _disp.setTextSize(isOk ? 2 : 1);
  _disp.setCursor(0, 0);

  // Mensaje principal: usar line1/line2 si vienen del RPC, si no el status
  if (line1.length() > 0 || line2.length() > 0) {
    _disp.setTextSize(2);  // texto principal más grande
    _disp.println(line1);
    _disp.println(line2);
  } else if (isOk) {
    _disp.setTextSize(2);
    _disp.println("ESTADO");
    _disp.println("   OK");
  } else {
    _disp.setTextSize(2);
    _disp.println("Alerta:");
    _disp.println(status);
  }

  // Datos adicionales si están presentes
  _disp.setTextSize(1);
  auto printIfValid = [&](const char* label, float value, int decimals = 1) {
    if (!isnan(value)) {
      _disp.print(label);
      _disp.print(": ");
      _disp.println(String(value, decimals));
    }
  };

  printIfValid("T(C)", tempC, 1);
  printIfValid("H(%)", humPct, 1);
  printIfValid("CO2", eco2Ppm, 0);
  printIfValid("TVOC", tvocPpb, 0);
  printIfValid("Lux", lux, 0);
  printIfValid("Noise", noiseDb, 1);

  _disp.display();
}
