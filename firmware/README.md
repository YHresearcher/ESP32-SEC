# ESP32-SEC firmware

Target: ESP32-S3 N16R8, ST7789V 240x320, XPT2046 resistive touch on Chibi-verified GPIO.

```sh
idf.py set-target esp32s3
idf.py build
```

Current status: development-only. The ESP-IDF GitHub Actions workflow should build the latest commit before any user flash. Use `docs/HARDWARE-TEST.md` to back up Chibi and validate this new firmware. A green CI check is **not** hardware certification.

Implemented source: touch UI, Wi-Fi discovery, BLE passive observer, channel statistics, conservative security observations, cooperative scan cancellation and NVS language preference. Full Vietnamese diacritic font, export/report and stable web-flasher manifest are release blockers.

Never use the stock ESP32-DIV binary on this distinct wiring profile.
