# ESP32-SEC PRD — implementation baseline

Version: 1.0 design review (2026-11-17)
Owner: YHresearcher

## Product goal
Build a handheld, local-first Wi-Fi and BLE security diagnostics toolkit for ESP32-S3-WROOM N16R8 and the owner's working 2.8-inch Chibi touchscreen, with a separate bilingual EN/VI USB flashing website.

## Sources
- ESP32-DIV: main architecture reference for wireless features and multipurpose UI.
- ESP32 Marauder: secondary Wi-Fi/BLE reference.
- ModESP32: source for board wiring and display/touch behavior previously validated by owner.

## MVP requirements
- FR01: boot stable and initialize LCD ST7789V 240x320, XPT2046 shared SPI.
- FR02: responsive touch UI home page with six menus: Wi-Fi, BLE, Defense, Analytics, Diagnostics, Settings.
- FR03: Wi-Fi discovery with SSID/BSSID, RSSI, channel and security type.
- FR04: BLE passive advertising discovery and RSSI without connecting.
- FR05: channel density and open-AP/duplicate-SSID *observations*, with no unsupported attack attribution.
- FR06: English/Vietnamese settings persisted to NVS. Full Vietnamese glyph font is required for stable release.
- FR07: diagnostics report actual PSRAM/heap and peripherals.
- FR08: website supports EN/VI, Web Serial and manual flashing of *hardware-verified* releases only.
- FR09: build CI artifacts are versioned, never automatically designated Stable.
- FR10: all scan operations bounded in duration, cancellable, and no UI lockups (target; full stop/cancel support is not yet implemented).
- FR11: export useful session data to JSON/CSV without leaking secrets (target, not implemented).

## Non-goals
No jamming, Wi-Fi deauth, rogue AP credential capture, BLE notification spam, spoofed tracker abuse, or unauthorized replay/injection. No additional NRF24/CC1101/IR/NFC/GPS hardware required for V1.

## Quality gates
1. ESP-IDF CI passes.
2. Real board boots repeatedly with correct color/touch.
3. Wi-Fi and BLE results plausible; 60-minute stability run.
4. EN/VI strings and full Vietnamese font verified.
5. A complete USB recovery procedure is tested after backing up Chibi.
6. Only then publish firmware binaries + install manifest.

See [ARCHITECTURE](ARCHITECTURE.md), [ACCEPTANCE](ACCEPTANCE.md), and [HARDWARE TEST](HARDWARE-TEST.md).

## Current gap
The committed implementation is an **unverified prototype**, not V1.0 Stable. Firmware CI must pass, font support and scan cancellation need completion, and real-device acceptance remains required.
