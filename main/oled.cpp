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

  // Helper to format values concisely (no decimals)
  auto fmt = [&](float v) -> String {
    if (!isfinite(v)) return String("--");
    return String((int)lroundf(v));
  };

  int height = _disp.height();
  int width = _disp.width();

  // Use larger text size (2). If display is small (height <= 32) we cannot fit
  // all 8 variables at size 2 on a single screen, so we show 4 per page and
  // cycle pages every 1500 ms. Taller displays show all variables at size 2.
  _disp.setTextSize(2);
  if (height <= 32) {
    // 7 variables + placeholder to place LUX in bottom-right: indices 0..7
  const char* labels[8] = {"T","H","P","CO2","TVOC","","dB","LUX"};
    String values[8];
    values[0] = fmt(d.temp);
    values[1] = fmt(d.hum);
    values[2] = fmt(d.press);
    values[3] = fmt(d.eco2);
    values[4] = fmt(d.tvoc);
    values[5] = d.hasNoise ? fmt(d.noiseDb) : String("--");
    values[6] = String("");
    values[7] = d.hasLight ? fmt(d.lux) : String("--");

    // Paging: 4 items per page (2 cols x 2 rows), page 0: 0-3, page 1: 4-7
    static uint8_t page = 0;
    static unsigned long lastSwitch = 0;
    const unsigned long pageMs = 1500;
    unsigned long now = millis();
    if (now - lastSwitch >= pageMs) {
      page = (page + 1) % 2;
      lastSwitch = now;
    }

    int startIdx = page * 4;
    int colW = width / 2;
    int xs[2] = {0, colW};
    int ys[2] = {0, 16};
    for (int i = 0; i < 4; ++i) {
      int idx = startIdx + i;
      int col = i % 2;
      int row = i / 2;
      int x = xs[col];
      int y = ys[row];
      _disp.setCursor(x, y);
      if (idx < 8 && labels[idx][0] != '\0') {
        _disp.print(labels[idx]);
        _disp.print(":");
        _disp.print(values[idx]);
      }
    }
  } else {
    // Taller displays (128x64) -> pair rows: left/right columns with last row for LUX
  const char* leftLabels[4] = {"T","H","P","dB"};
  const char* rightLabels[4] = {"CO2","TVOC","","LUX"};
  String leftValues[4];
  String rightValues[4];
  leftValues[0] = fmt(d.temp) + String("C");
  leftValues[1] = fmt(d.hum) + String("%");
  leftValues[2] = fmt(d.press);
  leftValues[3] = d.hasNoise ? fmt(d.noiseDb) : String("--");
  rightValues[0] = fmt(d.eco2);
  rightValues[1] = fmt(d.tvoc);
  rightValues[2] = String("");
  rightValues[3] = d.hasLight ? fmt(d.lux) : String("--");

    int mid = width / 2;
    for (int r = 0; r < 4; ++r) {
      int y = r * 16; // spacing for size 2
      _disp.setCursor(0, y);
      if (leftLabels[r][0] != '\0') {
        _disp.print(leftLabels[r]);
        _disp.print(":");
        _disp.print(leftValues[r]);
      }

      _disp.setCursor(mid, y);
      _disp.print(rightLabels[r]);
      _disp.print(":");
      _disp.print(rightValues[r]);
    }
  }

  _disp.display();
}
