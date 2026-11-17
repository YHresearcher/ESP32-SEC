# ESP32-SEC

Custom wireless security diagnostics firmware for **ESP32-S3 WROOM N16R8 (16 MB flash, 8 MB PSRAM)** with the owner's existing **ST7789V 240×320 + XPT2046** touchscreen. The display/touch pin profile follows a working [Chibi / ModESP32](https://github.com/YHresearcher/ModESP32) hardware build.

**Status: DEVELOPMENT / NO HARDWARE VERIFIED ESP32-SEC RELEASE.** This repository contains code, not a tested firmware image. Hardware validation of the original Chibi display does **not** validate this firmware.

## What is currently in the repository

| Feature | Implementation | Test |
|---|---|---|
| ESP-IDF firmware, N16R8 settings | Source committed | CI pending |
| LCD / touch, using Chibi GPIO | Source committed | Security Lab hardware test pending |
| Six-button LVGL UI | Source committed | Hardware pending |
| EN/VI settings stored in NVS | Source committed; Vietnamese ASCII fallback | Full diacritic font pending |
| Wi-Fi survey (APs, RSSI, channel, auth) | Source committed | CI + hardware pending |
| BLE passive advertisement observer | Source committed | CI + hardware pending |
| 2.4GHz channel counts and duplicate-SSID/open AP observations | Source committed | Hardware pending |
| Website EN/VI + Web Serial | Source committed | Deploy pending |
| Web installer | Installer shell, **disabled** absent verified manifest | No stable binary |
| Export, microSD, NRF24, CC1101, GPS | Not implemented | Future scope |

## Build

Use **ESP-IDF v6.0.1+**, Python dependencies and IDF Component Manager.

```sh
cd firmware
idf.py set-target esp32s3
idf.py build
```

To test on the real board, first back up the working Chibi firmware/NVS, verify USB ROM bootloader recovery and wiring, then use `idf.py -p <serial-port> flash monitor`. Use the 16 MB N16R8 partition profile. **Do not flash binary files from the original ESP32-DIV board.**

## Website

`web/` is an HTML/CSS/JS static site that supports EN/VI and Web Serial. GitHub Pages workflow is present; enabling GitHub Pages with *Source: GitHub Actions* may be necessary in repository Settings. Intended URL after deployment: https://yhresearcher.github.io/ESP32-SEC/ (not guaranteed live until deployment confirmed).

`web/manifest.json` intentionally does not exist. It is published only after a real board passes acceptance tests, verifying bootloader/partition/app offsets and a recovery procedure. Therefore the web installer does **not** offer a dangerous untested flash.

## Hardware pins (Chibi reference)

SCK 12; MOSI 11; MISO 8; LCD CS 10, DC 13, RST 14; XPT2046 CS 9, IRQ 7. LCD inversion false; touch X mirroring true. No backlight GPIO is driven because it has not been confirmed.

## Development policy

Read [Architecture](docs/ARCHITECTURE.md) and [Acceptance criteria](docs/ACCEPTANCE.md). Build success is not hardware verification. Explicitly exclude packet flooding, unauthorized disconnection, jamming, deceptive credential capture and hostile HID injection.

## Attribution

- [ESP32-DIV](https://github.com/cifertech/ESP32-DIV) — MIT licensed architecture reference (not directly vendored).
- [ESP32 Marauder](https://github.com/justcallmekoko/ESP32Marauder) — reference only.
- [ModESP32](https://github.com/YHresearcher/ModESP32) — working Chibi LCD/touch pin profile.


## ESP32-DIV integration (work in progress)

The [upstream ESP32-DIV](https://github.com/cifertech/ESP32-DIV) feature inventory and a hardware compatibility matrix are tracked in [docs/ESP32-DIV-INTEGRATION.md](docs/ESP32-DIV-INTEGRATION.md).

- The existing ESP32-S3 Wi-Fi and NimBLE passive scan integrations are retained. BLE scan results are now connected to the screen, and host synchronization is awaited before scanning. **Physical-device verification remains necessary.**
- A compiled feature registry records currently integrated firmware modules versus future NRF24, CC1101, IR, NFC/RFID, GNSS and microSD modules. Registry entries never imply that a missing peripheral is installed.
- A build-time Vietnamese LVGL 9 font generator rasterizes Unicode glyphs; it requires Pillow and DejaVu Sans on the CI runner. Source TTF files are **not** committed to the firmware image.
- Dangerous disruptive and impersonation modules are explicitly excluded. This is not a wholesale copy of ESP32-DIV Arduino code: the board uses ESP-IDF, LVGL 9, and an ST7789V screen, not upstream ILI9341 wiring.

**Release status:** experimental. CI compilation verifies builds, not on-device compatibility, radio performance, or touchscreen calibration.
