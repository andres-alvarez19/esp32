#include "oled.h"
#include <Arduino.h>

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

void OledView::drawStatus(const String& status) {
  if (!_ok) return;
  _disp.clearDisplay();

  bool isOk = status.equalsIgnoreCase("OK");
  _disp.setTextSize(isOk ? 2 : 1);
  _disp.setCursor(0, 0);

  if (isOk) {
    _disp.println("ESTADO");
    _disp.println("   OK");
  } else {
    _disp.println("Alerta:");
    _disp.println(status);
  }

  _disp.display();
}
