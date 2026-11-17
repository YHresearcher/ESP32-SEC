# ESP32-SEC architecture and current coverage

## Target
ESP32-S3-WROOM N16R8, LCD ST7789V 240x320 + XPT2046, shared SPI pins copied from user's working Chibi configuration. ESP-IDF 6.0.1+, LVGL 9, Espressif LVGL Port.

## Modules
- `board.c`: SPI2 LCD + shared XPT2046 touch, no guessed backlight GPIO.
- `ui.c`: 6-tile touch UI; NVS language selection; scrollable detail panel; Wi-Fi/BLE scan dispatched to workers.
- `wifi_scan.c`: standards-compliant AP scan, SSID/BSSID/channel/RSSI/auth.
- `ble_scan.c`: passive BLE advertisement observer using ESP-IDF NimBLE; bounded result array; time-limited scan.
- Analytics: per-channel counts and strongest RSSI from Wi-Fi scan results.
- Defense: open-AP and duplicate-SSID *observations*, not attack attribution.
- `web/`: static bilingual installer shell and Web Serial console. No `manifest.json` intentionally.
- `.github/workflows/build.yml`: CI-only binary compile (unverified).
- `.github/workflows/pages.yml`: GitHub Pages deployment (requires Pages setting: GitHub Actions).

## Current limitations and release gates
- Wi-Fi and BLE share the ESP32-S3 2.4GHz radio; this is a development prototype, not a validated coexistence scheduler.
- Scan results are volatile (RAM), no persistent capture or JSON/CSV export yet.
- Vietnamese locale UI currently uses unaccented ASCII fallbacks because the proper Vietnamese font has not been bundled. Website EN/VI is fully accented.
- No BLE connect/pair, unauthorized traffic injection, signal interference, deauth, impersonation or jamming features.
- No hardware smoke test or signed stable binary, no deployed installer manifest.

## Sources & credits
- https://github.com/cifertech/ESP32-DIV (MIT license; architectural reference)
- https://github.com/justcallmekoko/ESP32Marauder (reference only)
- https://github.com/YHresearcher/ModESP32 (board configuration inspiration; verify license before copying major source)
- Espressif IDF documentation for Wi-Fi and NimBLE; components installed via IDF Component Manager

The code in ESP32-SEC is a new modular implementation and does not directly vendor upstream security-tool modules.
