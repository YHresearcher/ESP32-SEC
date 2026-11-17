# ESP32-SEC

ESP32-S3-WROOM N16R8 wireless security lab, using the 2.8-inch ST7789V + XPT2046 touchscreen validated on the owner's Chibi device.

**Status: M1/M2 source foundation, NOT yet compiled or tested on ESP32-SEC hardware.** No release binary is available. Wi-Fi/BLE scanners and the web installer are not implemented in this milestone.

Architecture: ESP-IDF + LVGL, with selected architectural ideas from [ESP32-DIV](https://github.com/cifertech/ESP32-DIV) and [ESP32 Marauder](https://github.com/justcallmekoko/ESP32Marauder). No offensive interference or disruption functionality included.

Firmware source: `firmware/`. Build requires ESP-IDF 6.0.1+, then `cd firmware && idf.py set-target esp32s3 && idf.py build`. Do not flash without checking the USB recovery procedure and backing up your working Chibi firmware.

For pinout, test scope, and unverified conditions, see `docs/ACCEPTANCE.md`.
