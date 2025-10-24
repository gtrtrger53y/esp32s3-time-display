/*
 * Simple New York Time Display for the Waveshare ESP32‑S3‑LCD‑1.47B
 *
 * This sketch connects to a Wi‑Fi network, synchronises the on‑board RTC
 * against NTP servers and then renders the current time (Eastern Time)
 * onto the built‑in 1.47″ ST7789 LCD.  The time is shown in 12‑hour format
 * with an AM/PM suffix and updates every half second.  The code makes
 * minimal use of dynamic memory and does not allocate any buffers on the
 * heap.
 *
 * Hardware pin assignment is taken from the Waveshare wiki.  If you are
 * building against another board with a ST7789 display, update the
 * definitions below to match your hardware.  The Arduino_GFX_Library
 * handles the low‑level SPI configuration for the display.
 */
#include <WiFi.h>
#include <time.h>
#include <Arduino_GFX_Library.h>

// TODO: Replace these with your actual Wi‑Fi credentials before building.
const char* WIFI_SSID     = "M10WiFi";
const char* WIFI_PASSWORD = "8326896452";

// Pin mapping for the ESP32‑S3‑LCD‑1.47B.
// See: https://www.waveshare.com/wiki/ESP32-S3-LCD-1.47B
static const uint8_t TFT_CS   = 42;
static const uint8_t TFT_DC   = 41;
static const uint8_t TFT_RST  = 39;
static const uint8_t TFT_BL   = 46;
static const uint8_t TFT_MOSI = 45;
static const uint8_t TFT_SCLK = 40;

// Create an SPI bus and display driver.  The final two arguments to
// Arduino_ST7789 are the panel width and height.  The built‑in display
// is 172×320 pixels, oriented in portrait mode.  The rotation value of
// 3 (270°) turns the screen so that the USB‑C connector is at the bottom.
Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, -1 /* MISO not used */);
Arduino_GFX     *gfx = new Arduino_ST7789(bus, TFT_RST, 3 /* rotation */, true /* IPS */, 172 /* width */, 320 /* height */);

// Helper to connect to Wi‑Fi.  Blocks until connected.
void connectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    // give up after 15 seconds
    if (millis() - startAttemptTime > 15000) {
      break;
    }
    delay(250);
  }
}

// Setup runs once on boot.
void setup() {
  // Initialise the backlight and turn it on.  The backlight is a plain
  // digital pin; driving it HIGH powers the LED array.  For brightness
  // control you could use ledc analog dimming instead of simple digitalWrite.
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // Initialise display hardware.
  gfx->begin();
  gfx->fillScreen(BLACK);
  gfx->setTextColor(WHITE, BLACK);

  // Connect to the Wi‑Fi network.  This call is non‑blocking after
  // connection is established.
  connectToWiFi();

  // Configure the timezone for America/New_York.  The TZ string is
  // documented in "man tzset".  EST5EDT means Eastern Standard Time with
  // daylight saving.  M3.2.0/2 means the second Sunday in March at 02:00
  // UTC starts DST.  M11.1.0/2 means the first Sunday in November at
  // 02:00 UTC ends DST.
  setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0/2", 1);
  tzset();

  // Synchronise time via NTP.  The first two arguments (gmtOffset_sec,
  // daylightOffset_sec) are ignored when a timezone is set using
  // setenv/tzset, but remain for backwards compatibility with older
  // examples.  Multiple servers improve reliability.  configTime() starts
  // asynchronous NTP synchronisation.
  configTime(0, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");
}

// Draw the current time centred on the screen.  This helper clears
// the previous time and redraws it to minimise flicker.  You can
// customise the font and size here.
void drawTime(const struct tm *tm_info) {
  // Convert 24‑hour value to 12‑hour and choose AM/PM suffix.
  int hour24 = tm_info->tm_hour;
  int hour12 = hour24 % 12;
  if (hour12 == 0) hour12 = 12;
  const char *ampm = (hour24 >= 12) ? "PM" : "AM";

  char buf[16];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d %s", hour12, tm_info->tm_min, tm_info->tm_sec, ampm);

  // Clear the screen and draw the time.  You could instead draw a
  // coloured rectangle only over the time area to reduce flicker.
  gfx->fillScreen(BLACK);
  gfx->setTextSize(4);        // each glyph is 8x16 * scale factor
  int16_t x, y;
  uint16_t w, h;
  gfx->getTextBounds(buf, 0, 0, &x, &y, &w, &h);
  // centre horizontally and vertically
  int16_t cx = (gfx->width()  - w) / 2;
  int16_t cy = (gfx->height() - h) / 2;
  gfx->setCursor(cx, cy);
  gfx->print(buf);
}

// Main loop: update the display every 500 ms.  The NTP client
// periodically updates the system time in the background once Wi‑Fi is
// connected.  If Wi‑Fi is not available, the time will be based on the
// last successful sync or the built‑in RTC.
void loop() {
  time_t now = time(NULL);
  struct tm timeinfo;
  if (localtime_r(&now, &timeinfo)) {
    drawTime(&timeinfo);
  }
  // Delay for half a second to limit refresh rate and reduce flicker.
  delay(500);
}
