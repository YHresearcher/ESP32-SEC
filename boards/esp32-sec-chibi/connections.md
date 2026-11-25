# ESP32-SEC Chibi — wiring reference

## Board

ESP32-S3-WROOM-1-N16R8 (16 MB quad flash, 8 MB octal PSRAM) with an
ST7789V 240x320 TFT and an XPT2046 resistive touch panel, following the
owner's working ModESP32/Chibi display build.

## Hardware-verified pins (from the Chibi profile)

| Signal | GPIO |
|---|---|
| SPI SCLK | 12 |
| SPI MOSI | 11 |
| SPI MISO | 8 |
| LCD CS | 10 |
| LCD DC | 13 |
| LCD RST | 14 |
| XPT2046 CS | 9 |
| XPT2046 IRQ | 7 |
| LCD backlight | not driven — GPIO unverified |

The LCD and the XPT2046 **share the same SPI bus**. Bruce initialises touch
with `hal_touch_init(..., xpt_shared_spi = true)`, which reuses `tft`'s SPI
instance and its transaction lock.

**Rotation:** the UI runs at rotation 1 (320x240 landscape). The ST7789
inversion command is off — set at compile time with
`-DDEFAULT_COLOR_INVERTED=0` (Bruce loads stored settings *after* the board's
GPIO hook, so a runtime assignment alone would be overwritten by NVS).

`TFT_eSPI` adds a CGRAM offset only for 240x240, 135x240, 172x320, 170x320
and 240x300 panels; a 240x320 panel needs none, so no `CGRAM_OFFSET` flag is
set. If red and blue look swapped, change `TFT_RGB_ORDER` to `TFT_BGR`. If the
display glitches at 40 MHz, lower `SPI_FREQUENCY` to 20000000.

## GPIOs reserved by the module — do not use

- **GPIO33–GPIO37**: octal PSRAM data/DQS on N16R8. Not broken out and unsafe
  to touch in software.
- **GPIO26–GPIO32**: in-package flash SPI.
- **GPIO19 / GPIO20**: native USB (Serial and BadUSB) in this build.

## Assumed defaults for external modules

These are **not** soldered on the stock board. They are Bruce's startup
defaults and can be changed at runtime via **Config > Set Device pins**;
the saved `/brucePins.conf` then takes precedence on every later boot.

| Module | Pins (sck/miso/mosi/cs[/extra]) | Notes |
|---|---|---|
| I2C (Grove) | SDA 4, SCL 5 | PN532-I2C, ST25R3916, RTC, sensors |
| UART / GPS | RX 44, TX 43 | GPS module at 9600 baud by default |
| CC1101 | 12 / 8 / 11 / 6 / GDO0 15 | sub-GHz RF tools |
| NRF24 | 12 / 8 / 11 / CS 16 / CE 17 | 2.4 GHz tools |
| PN532 (SPI) / RC522 | 12 / 8 / 11 / CS 6 | RFID/NFC |
| ST25R3916 | 12 / 8 / 11 / CS 6 | high-frequency NFC |
| microSD (SPI) | 12 / 8 / 11 / CS 21 | share the display bus |
| W5500 | 12 / 8 / 11 / CS 18 / INT 1 / RST 2 | wired Ethernet |
| LoRa | 12 / 8 / 11 / CS 38 / IRQ 39 / RST 40 | optional |
| IR TX / RX | GPIO 6 / 16 (via Config) | no fixed wiring; `ALLOW_ALL_GPIO_FOR_IR_RF` is on |
| 433 MHz RF TX / RX | GPIO 6 / 16 (via Config) | one-pin modules |

Several SPI peripherals share one CS (6) because they are not used
simultaneously. Give each module its own CS if you wire more than one at once.

## Touch calibration

The transform arrays in `interface.cpp` are a starting estimate for a panel
that is natively portrait but rendered landscape. On first boot, run
**Config > Calibrate Touch**, tap all targets, and let Bruce store the result
in NVS. If touches are mirrored or rotated, adjust `swapXY` / `mirrorX` /
`mirrorY` for rotation 1 in `touchCfg()` and rebuild.

## Build / flash

```sh
pio run -e esp32-sec-chibi
pio run -e esp32-sec-chibi -t upload
```

Use a 16 MB flash / 8 MB PSRAM device. Back up any existing firmware and NVS
before flashing, and confirm a ROM-bootloader recovery path.
