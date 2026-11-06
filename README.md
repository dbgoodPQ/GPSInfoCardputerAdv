# GPS Info for Cardputer ADV – External Display

This project is based on [DevinWatson/Cardputer-Adv-GPS-Info](https://github.com/DevinWatson/Cardputer-Adv-GPS-Info).
It adds support for an external **Waveshare 2.0inch LCD Module** (ST7789, 320x240) as an alternative to the Cardputer ADV's built-in screen.

## Screen pinout

| Waveshare 2.0" pin | Cardputer ADV | Notes |
|---|---|---|
| VCC | 3V3 | set the module's voltage selector to 3.3 V |
| GND | GND | |
| DIN | G14 | shared with the microSD card (MOSI) |
| CLK | G40 | shared with the microSD card (SCLK) |
| CS  | G5  | |
| DC  | G6  | |
| RST | G3  | |
| BL  | G4  | backlight, PWM brightness |

GPS module on the Grove port: GPS TX → G1, GPS RX → G2, 9600 baud.

## Changes from the original

- **External display mode.** Output goes to the Waveshare 2.0" LCD instead of the built-in 240x135 screen; the built-in screen is turned off while it is in use.
- **Native 320x240 layouts.** Every screen and popup is laid out for 320x240 and drawn with bitmap fonts at their real pixel size. Nothing is scaled or zoomed, so text and graphics stay sharp. Built-in screen mode looks exactly as in the original.
- **Config file read at boot.** `/cpGpsInfo.conf` on the microSD card selects the screen and holds the GPS and LCD settings. Copy [`cpGpsInfo.conf.example`](cpGpsInfo.conf.example) to the card as `cpGpsInfo.conf`. If the file is missing, a default one is created (`display=internal`).

  | Key | Default | Meaning |
  |---|---|---|
  | `display` | `internal` | `internal` or `external` |
  | `gps_rx`, `gps_tx`, `gps_baud` | `1`, `2`, `9600` | GPS UART |
  | `ext_pin_mosi/sclk/cs/dc/rst/bl` | `14/40/5/6/3/4` | external LCD wiring |
  | `ext_rotation` | `1` | `1` landscape, `3` landscape upside down |
  | `ext_invert` | `1` | set `0` if colours look inverted |
  | `ext_bgr` | `0` | set `1` if red and blue are swapped |
  | `ext_brightness` | `200` | 0–255 |
  | `ext_spi_mhz` | `40` | lower (e.g. 20) for long jumper wires |
  | `int_brightness` | `128` | built-in screen brightness |

- **GPS settings are saved.** Pressing `[ok]` in the `[c]` configuration menu writes the GPS RX/TX pins and baud rate to `/cpGpsInfo.conf`, keeping the other settings in the file.
- **New GPS defaults.** GPS on the Grove port (G1/G2) at 9600 baud.
- **Screenshots** over USB serial (`[Enter]`) use the active resolution (320x240 in external mode).

## Thanks

Many thanks to **Devin Watson** for [Cardputer-Adv-GPS-Info](https://github.com/DevinWatson/Cardputer-Adv-GPS-Info), the Cardputer ADV version with its 16 screens, world map and 3D globe that this project builds on. Thanks also to **alcor55**, author of the original [Cardputer-GPS-Info](https://github.com/alcor55/Cardputer-GPS-Info) it grew from.
