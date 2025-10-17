#include "oled.h"
#include <Arduino.h>

bool OledView::begin(uint8_t addr) {
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

void OledView::draw(const EnvData& d, float co2MaxPpm) {
  if (!_ok) return;
  _disp.clearDisplay();

  _disp.setTextSize(1);
  _disp.setCursor(0, 0);
  _disp.print("T:");
  _disp.print(d.temp, 1);
  _disp.print("C  H:");
  _disp.print(d.hum, 1);
  _disp.println("%");

  _disp.setTextSize(1);
  _disp.setCursor(0, 16);
  _disp.print("CO2: ");
  _disp.print(d.eco2, 0);
  _disp.println(" ppm");

  const char* level = "BUENO";
  if (d.eco2 > 1200) level = "MALO";
  else if (d.eco2 > 800) level = "REGULAR";
  _disp.setCursor(0, 32);
  _disp.print("Nivel: ");
  _disp.println(level);

  _disp.display();
}
