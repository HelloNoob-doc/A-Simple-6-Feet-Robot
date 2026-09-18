#include <TFT_eSPI.h>

TFT_eSPI tft;

constexpr uint32_t STATUS_REFRESH_MS = 1000;

void enableBacklight() {
#if defined(TFT_BL) && defined(TFT_BACKLIGHT_ON)
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
#endif
}

void drawStaticContent() {
  const int screenWidth = tft.width();
  constexpr uint16_t colors[] = {
    TFT_RED, TFT_GREEN, TFT_BLUE, TFT_WHITE, TFT_BLACK
  };
  constexpr int colorCount = sizeof(colors) / sizeof(colors[0]);
  const int barY = 62;
  const int barHeight = 24;
  const int barWidth = screenWidth / colorCount;

  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("Baize ESP32", 8, 8, 4);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("TFT_eSPI configured", 8, 40, 2);

  for (int i = 0; i < colorCount; ++i) {
    tft.fillRect(i * barWidth, barY, barWidth, barHeight, colors[i]);
  }
  tft.drawRect(0, barY, screenWidth, barHeight, TFT_DARKGREY);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("RGBW colour test", 8, 94, 2);
}

void drawStatus() {
  static uint32_t lastUpdate = 0;
  const uint32_t now = millis();

  if (now - lastUpdate < STATUS_REFRESH_MS) {
    return;
  }
  lastUpdate = now;

  tft.fillRect(8, 114, tft.width() - 16, 16, TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(8, 114, 2);
  tft.printf("Running: %lus", static_cast<unsigned long>(now / 1000));
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // Pin definitions come from the TFT_eSPI User_Setup selected by the user.
  enableBacklight();
  tft.init();
//   tft.setRotation(3);

  drawStaticContent();
  drawStatus();

  Serial.printf("TFT ready: %d x %d\n", tft.width(), tft.height());
}

void loop() {
  drawStatus();
}