/*
 *  display_config.h
 *
 *  Boot-time configuration file and output display selection for
 *  Cardputer ADV GPS Info.
 *
 *  - Reads /cpGpsInfo.conf from the microSD card once at boot.
 *  - Selects the output display:
 *      display=internal  -> built-in 240x135 ST7789V2 (original behaviour)
 *      display=external  -> Waveshare 2.0" LCD (ST7789VW, 240x320 IPS),
 *                           used in landscape at 320x240
 *  - When the external display is active, the built-in display is put to
 *    sleep with its backlight off.
 *
 *  Bus notes (Cardputer ADV):
 *    Built-in display: SPI3 (SCLK G36, MOSI G35, DC G34, CS G37, RST G33, BL G38)
 *    microSD card    : SCLK G40, MOSI G14, MISO G39, CS G12
 *    External LCD    : SPI2 on the SD lines (SCLK G40, MOSI G14) + own CS/DC/RST/BL
 *  The config file is read from SD *before* the external LCD takes the shared
 *  SCLK/MOSI lines, then SD is released and its CS (G12) is held HIGH so the
 *  card ignores display traffic.
 *
 *  This file is included from Cardputer_GPS_Info.ino after the global
 *  variables it uses (gpsRxPin, gpsTxPin, gpsBaud, screenW, screenH, frameBuf).
 */
#pragma once

#ifndef HOST_PREVIEW
  #include <SD.h>
  #include <SPI.h>
  #include <lgfx/v1/panel/Panel_ST7789.hpp>
#endif

#define CONFIG_PATH "/cpGpsInfo.conf"

// microSD pins on the Cardputer ADV.
#define SD_PIN_SCLK 40
#define SD_PIN_MISO 39
#define SD_PIN_MOSI 14
#define SD_PIN_CS   12

enum DisplayMode { DISPLAY_INTERNAL = 0, DISPLAY_EXTERNAL = 1 };

struct AppConfig {
  DisplayMode display   = DISPLAY_INTERNAL;
  // Waveshare 2.0" LCD wiring (defaults = wiring documented in README).
  int  extPinMosi       = 14;   // DIN  (shared with SD MOSI)
  int  extPinSclk       = 40;   // CLK  (shared with SD SCLK)
  int  extPinCs         = 5;
  int  extPinDc         = 6;
  int  extPinRst        = 3;
  int  extPinBl         = 4;    // backlight, PWM
  int  extRotation      = 1;    // 1 = landscape, 3 = landscape flipped 180
  bool extInvert        = true; // IPS ST7789 panels need colour inversion
  bool extBgr           = false;// set true if red and blue are swapped
  int  extBrightness    = 200;  // 0-255
  int  extSpiMhz        = 40;   // SPI write clock
  int  intBrightness    = 128;  // built-in display brightness 0-255
};

AppConfig appCfg;
bool extDisplay      = false;   // true once the external display is driving output
bool configFromSD    = false;   // config file was found and read
bool configCreated   = false;   // a default config file was written this boot
String displayNote   = "";      // short reason shown in the config popup

// Display that receives the frame buffer.
lgfx::LovyanGFX* gfxOut = nullptr;

// ------------------------------------------------------------------
//  External LCD device (Waveshare 2.0inch LCD Module, ST7789VW)
// ------------------------------------------------------------------
#ifndef HOST_PREVIEW
class LGFX_Waveshare20 : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_SPI      _bus;
  lgfx::Light_PWM    _light;
public:
  void configure(const AppConfig &c) {
    {
      auto cfg = _bus.config();
      cfg.spi_host   = SPI2_HOST;        // built-in display owns SPI3.
      cfg.spi_mode   = 0;
      cfg.freq_write = (uint32_t)c.extSpiMhz * 1000000UL;
      cfg.freq_read  = 16000000;
      cfg.spi_3wire  = false;
      cfg.use_lock   = true;
      cfg.pin_sclk   = c.extPinSclk;
      cfg.pin_mosi   = c.extPinMosi;
      cfg.pin_miso   = -1;               // panel is write-only here.
      cfg.pin_dc     = c.extPinDc;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs           = c.extPinCs;
      cfg.pin_rst          = c.extPinRst;
      cfg.pin_busy         = -1;
      cfg.panel_width      = 240;
      cfg.panel_height     = 320;
      cfg.memory_width     = 240;
      cfg.memory_height    = 320;
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      cfg.offset_rotation  = 0;
      cfg.readable         = false;
      cfg.invert           = c.extInvert;
      cfg.rgb_order        = c.extBgr;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = false;   // SD is released before init.
      _panel.config(cfg);
    }
    if (c.extPinBl >= 0) {
      auto cfg = _light.config();
      cfg.pin_bl      = c.extPinBl;
      cfg.invert      = false;
      cfg.freq        = 12000;
      cfg.pwm_channel = 6;            // built-in backlight uses channel 7.
      _light.config(cfg);
      _panel.setLight(&_light);
    }
    setPanel(&_panel);
  }
};
LGFX_Waveshare20 extLcd;
#endif

// ------------------------------------------------------------------
//  Config file parsing
// ------------------------------------------------------------------

static bool cfgParseBool(String v) {
  v.toLowerCase();
  return (v == "1" || v == "true" || v == "yes" || v == "on");
}

/*    Apply one "key = value" pair. Unknown keys are ignored.
*/
void applyConfigPair(String key, String val) {
  key.trim(); val.trim();
  key.toLowerCase();
  if (key.length() == 0) return;
  if (key == "display") {
    String v = val; v.toLowerCase();
    appCfg.display = (v == "external" || v == "ext" || v == "waveshare")
                     ? DISPLAY_EXTERNAL : DISPLAY_INTERNAL;
  }
  else if (key == "ext_pin_mosi")   appCfg.extPinMosi    = val.toInt();
  else if (key == "ext_pin_sclk")   appCfg.extPinSclk    = val.toInt();
  else if (key == "ext_pin_cs")     appCfg.extPinCs      = val.toInt();
  else if (key == "ext_pin_dc")     appCfg.extPinDc      = val.toInt();
  else if (key == "ext_pin_rst")    appCfg.extPinRst     = val.toInt();
  else if (key == "ext_pin_bl")     appCfg.extPinBl      = val.toInt();
  else if (key == "ext_rotation")   appCfg.extRotation   = (val.toInt() == 3) ? 3 : 1;
  else if (key == "ext_invert")     appCfg.extInvert     = cfgParseBool(val);
  else if (key == "ext_bgr")        appCfg.extBgr        = cfgParseBool(val);
  else if (key == "ext_brightness") appCfg.extBrightness = constrain(val.toInt(), 0, 255);
  else if (key == "ext_spi_mhz")    appCfg.extSpiMhz     = constrain(val.toInt(), 1, 80);
  else if (key == "int_brightness") appCfg.intBrightness = constrain(val.toInt(), 0, 255);
  else if (key == "gps_rx")         gpsRxPin             = val.toInt();
  else if (key == "gps_tx")         gpsTxPin             = val.toInt();
  else if (key == "gps_baud")       gpsBaud              = val.toInt();
}

/*    Parse a whole config text: one "key=value" per line, '#' comments.
*/
void parseConfigText(const String &text) {
  int start = 0;
  while (start < (int)text.length()) {
    int end = text.indexOf('\n', start);
    if (end < 0) end = text.length();
    String line = text.substring(start, end);
    start = end + 1;
    int hash = line.indexOf('#');
    if (hash >= 0) line = line.substring(0, hash);
    int eq = line.indexOf('=');
    if (eq <= 0) continue;
    applyConfigPair(line.substring(0, eq), line.substring(eq + 1));
  }
}

const char* defaultConfigText() {
  return
    "# Cardputer ADV GPS Info - configuration (read once at boot)\n"
    "#\n"
    "# Output display:\n"
    "#   internal = built-in 240x135 screen\n"
    "#   external = Waveshare 2.0inch LCD (ST7789, used at 320x240)\n"
    "display=internal\n"
    "\n"
    "# GPS UART (Grove port: G1/G2). Swap rx/tx if no data is received.\n"
    "gps_rx=1\n"
    "gps_tx=2\n"
    "gps_baud=115200\n"
    "\n"
    "# External LCD wiring (Cardputer ADV GPIO numbers).\n"
    "# DIN and CLK share the microSD bus.\n"
    "ext_pin_mosi=14\n"
    "ext_pin_sclk=40\n"
    "ext_pin_cs=5\n"
    "ext_pin_dc=6\n"
    "ext_pin_rst=3\n"
    "ext_pin_bl=4\n"
    "\n"
    "# External LCD options.\n"
    "# ext_rotation: 1 = landscape, 3 = landscape upside down\n"
    "ext_rotation=1\n"
    "ext_invert=1\n"
    "ext_bgr=0\n"
    "ext_brightness=200\n"
    "ext_spi_mhz=40\n"
    "\n"
    "# Built-in screen brightness (internal mode).\n"
    "int_brightness=128\n";
}

/*    Read /cpGpsInfo.conf from microSD. If the card is present but the file
*     is missing, write a default one. The SD card is released afterwards.
*/
void loadConfigFromSD() {
#ifndef HOST_PREVIEW
  SPIClass sdSPI(FSPI);
  sdSPI.begin(SD_PIN_SCLK, SD_PIN_MISO, SD_PIN_MOSI, SD_PIN_CS);
  if (!SD.begin(SD_PIN_CS, sdSPI, 20000000)) {
    Serial.println("[cfg] No microSD card: using built-in defaults.");
    sdSPI.end();
    return;
  }
  if (SD.exists(CONFIG_PATH)) {
    File f = SD.open(CONFIG_PATH, FILE_READ);
    if (f) {
      String text = f.readString();
      f.close();
      parseConfigText(text);
      configFromSD = true;
      Serial.println("[cfg] Loaded " CONFIG_PATH);
    }
  } else {
    File f = SD.open(CONFIG_PATH, FILE_WRITE);
    if (f) {
      f.print(defaultConfigText());
      f.close();
      configCreated = true;
      parseConfigText(defaultConfigText());
      Serial.println("[cfg] Created default " CONFIG_PATH);
    }
  }
  SD.end();
  sdSPI.end();
#endif
}

/*    Reject external pin sets that collide with the built-in display,
*     the keyboard I2C bus or the GPS UART pins.
*/
bool extPinsValid(String &why) {
  int pins[] = { appCfg.extPinMosi, appCfg.extPinSclk, appCfg.extPinCs,
                 appCfg.extPinDc,   appCfg.extPinRst,  appCfg.extPinBl };
  const int reserved[] = { 33, 34, 35, 36, 37, 38,   // built-in display
                           8, 9 };                     // internal I2C (keyboard, IMU)
  for (int i = 0; i < 6; i++) {
    if (pins[i] == -1 && (i == 4 || i == 5)) continue; // RST and BL may be unused.
    if (pins[i] < 0 || pins[i] > 48) { why = "bad ext pin"; return false; }
    for (int r : reserved)
      if (pins[i] == r) { why = "ext pin " + String(pins[i]) + " reserved"; return false; }
    if (pins[i] == gpsRxPin || pins[i] == gpsTxPin) { why = "ext pin = GPS pin"; return false; }
    for (int j = 0; j < i; j++)
      if (pins[i] == pins[j]) { why = "ext pins repeat"; return false; }
  }
  return true;
}

// ------------------------------------------------------------------
//  Display bring-up and frame push
// ------------------------------------------------------------------

/*    Select and start the output display, then size and allocate frameBuf.
*/
void initDisplays() {
#ifndef HOST_PREVIEW
  gfxOut = &M5Cardputer.Display;
  if (appCfg.display == DISPLAY_EXTERNAL) {
    String why;
    if (!extPinsValid(why)) {
      displayNote = why;
      Serial.printf("[disp] External display disabled: %s\n", why.c_str());
    } else {
      // Keep the SD card deselected so it ignores traffic on the shared lines.
      pinMode(SD_PIN_CS, OUTPUT);
      digitalWrite(SD_PIN_CS, HIGH);
      extLcd.configure(appCfg);
      if (extLcd.init()) {
        extLcd.setRotation(appCfg.extRotation);
        extLcd.setBrightness(appCfg.extBrightness);
        extLcd.fillScreen(TFT_BLACK);
        gfxOut = &extLcd;
        extDisplay = true;
        // Built-in screen off.
        M5Cardputer.Display.fillScreen(TFT_BLACK);
        M5Cardputer.Display.setBrightness(0);
        M5Cardputer.Display.sleep();
        Serial.printf("[disp] External %dx%d active\n", extLcd.width(), extLcd.height());
      } else {
        displayNote = "ext init failed";
        Serial.println("[disp] External display init failed, using built-in.");
      }
    }
  }
  if (!extDisplay) {
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setBrightness(appCfg.intBrightness);
  }
  screenW = gfxOut->width();
  screenH = gfxOut->height();
  Serial.printf("[disp] %dx%d, heap free %u, largest block %u\n", screenW, screenH,
                (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMaxAllocHeap());
#endif
  // Full-colour frame buffer. 320x240x16bit needs 150 KB of contiguous RAM;
  // fall back to 8-bit colour (75 KB) if that is not available.
  frameBuf.setColorDepth(16);
  if (!frameBuf.createSprite(screenW, screenH)) {
    frameBuf.setColorDepth(8);
    if (frameBuf.createSprite(screenW, screenH))
      Serial.println("[disp] Low RAM: frame buffer in 8-bit colour.");
    else
      Serial.println("[disp] ERROR: frame buffer allocation failed.");
  }
}

/*    Push the off-screen frame to the active display.
*/
inline void pushFrame() {
  if (gfxOut) frameBuf.pushSprite(gfxOut, 0, 0);
}

// ------------------------------------------------------------------
//  Text helpers
// ------------------------------------------------------------------

/*    Small UI font for code shared by both modes (map, globe).
*     Internal: original 6x8 font. External: crisp 8x16 bitmap font.
*/
inline void uiFontSmall() {
  frameBuf.setFont(extDisplay ? (const lgfx::IFont*)&fonts::AsciiFont8x16
                              : (const lgfx::IFont*)&fonts::Font0);
  frameBuf.setTextSize(1);
}
