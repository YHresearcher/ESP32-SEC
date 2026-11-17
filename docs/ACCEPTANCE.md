# Acceptance gates

## Hardware baseline
Owner confirms the working Chibi screen + touch. ESP32-SEC adapts settings from `YHresearcher/ModESP32` ST7789V touch target, but **this firmware itself has not been hardware tested**.
- ST7789V 240x320: SPI SCK 12, MOSI 11, MISO 8, LCD CS 10, DC 13, RST 14.
- XPT2046: shared SPI, Touch CS 9, IRQ 7, provisional mirror X = true.
- LCD inversion OFF; backlight GPIO NOT controlled.
- 16 MB flash, 8 MB octal PSRAM expected.

## CI build vs hardware verification
- [ ] ESP-IDF 6.0.1 Component Manager dependencies resolve.
- [ ] Compile and link `firmware/` and inspect artifact/partition offsets.
- [ ] Boot after backing up working Chibi, verify USB recovery.
- [ ] LCD normal colors/rotation; touch 5 points, no ghost touches.
- [ ] 20 reboot test and 60-minute app stress.
- [ ] EN/VI UI navigation and persistent language settings.
- [ ] Vietnamese diacritics properly rendered (not implemented in current development snapshot).
- [ ] Wi-Fi scan completes without UI freeze, result RSSI/channel/security accuracy.
- [ ] BLE scanning (not implemented).
- [ ] Defender alerts (not implemented).
- [ ] No unapproved transmissions; ensure scanning is restricted to authorized assessment.
- [ ] Web flasher release manifest is checked against actual flash offsets; verify recovery procedure.

## Current status
The repo is a **development snapshot**. Wi-Fi AP discovery is written. BLE, passive analytics and full Vietnamese font remain incomplete. Do not label this a stable release.
