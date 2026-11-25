# ESP32-SEC — Bruce-based offensive firmware for the Chibi ESP32-S3

This repository is now a **fork of [BruceDevices/firmware](https://github.com/BruceDevices/firmware)**,
retargeted to the owner's ESP32-S3-WROOM-1-N16R8 handheld with an ST7789V
240x320 touchscreen and XPT2046 controller.

> **Legal / authorized use only.** Bruce is a red-team tool. Use this firmware
> only on networks, devices and systems you own or are explicitly authorized
> to test. Do not use it for unauthorized access, disruption or credential
> theft. See the upstream [disclaimer](https://github.com/BruceDevices/firmware#-disclaimer).

## What changed from the previous ESP32-SEC

The earlier repository was a passive Wi-Fi/BLE diagnostics prototype built
with ESP-IDF + LVGL, and its PRD explicitly excluded deauth, rogue APs,
jamming, credential capture and HID injection. That direction has been
**replaced**. This tree is the full Bruce codebase (PlatformIO/Arduino),
its complete offensive feature set, re-based onto this board:

- Full Bruce module set: Wi-Fi, BLE, sub-GHz RF (CC1101), NRF24, RFID/NFC
  (PN532 / ST25R3916 / RC522), IR, FM radio, BadUSB, GPS, scripts,
  web interface and the JS interpreter — no `LITE_VERSION` trimming.
- One board port only: `esp32-sec-chibi`, with the Chibi GPIO profile,
  16 MB flash and 8 MB octal PSRAM. The 53 unused upstream board ports were
  removed from the tree.
- AGPL-3.0, matching upstream. The previous defensive sources and docs were
  removed from the tree; the old `web/` flashing site remains but is not yet
  updated for these builds.

## Hardware target

| Part | Detail |
|---|---|
| MCU | ESP32-S3-WROOM-1-N16R8 — 16 MB flash, 8 MB octal PSRAM |
| Display | ST7789V, 240x320, SPI: SCLK 12 / MOSI 11 / MISO 8 / CS 10 / DC 13 / RST 14 |
| Touch | XPT2046, shared SPI, CS 9, IRQ 7 |
| Backlight | not driven (GPIO unverified on the Chibi build) |
| Rotation | 1 — Bruce UI runs landscape (320x240); panel inversion off |

GPIO33–37 are reserved by octal PSRAM and are never assigned. The display and
touch pins above come from the owner's working Chibi build; every other pin
(defaults for CC1101, NRF24, PN532, SD, GPS, IR, RF, etc.) is an assumption
that can be remapped from **Config > Set Device pins**. See
[boards/esp32-sec-chibi/connections.md](boards/esp32-sec-chibi/connections.md)
for the full table and touch-calibration notes.

## Build

Install [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html)
with **Python 3.10–3.14** (the pioarduino platform refuses Python 3.9), and
the Git submodules (all seven vendored libraries are tracked as gitlinks):

```sh
git clone --recurse-submodules <this-repo>
cd ESP32-SEC
git submodule update --init --recursive
pio run -e esp32-sec-chibi
```

`default_envs` in `platformio.ini` is already set to `esp32-sec-chibi`, so a
plain `pio run` builds the same target. Flash over the native USB port:

```sh
pio run -e esp32-sec-chibi -t upload
pio device monitor
```

First-boot checklist:

1. Back up the existing Chibi firmware and NVS, and verify ROM bootloader
   recovery before flashing.
2. Confirm the screen initialises in landscape and colours are correct.
3. Run **Config > Calibrate Touch** and save the result. If touches are
   mirrored/rotated, fix `touchCfg()` in
   [interface.cpp](boards/esp32-sec-chibi/interface.cpp) and rebuild.
4. Only then attach external RF/NFC/IR hardware, one module at a time.

A successful compile produces `Bruce-esp32-sec-chibi.bin` at the repository
root (a merged image ready for offset 0x0) in addition to the normal
`.pio/build/esp32-sec-chibi/` artifacts. A successful compile does **not**
mean on-device verification. Radio behaviour, touch calibration and
peripheral wiring still require hardware testing.

## Attribution and licence

This fork is derived from Bruce by the BruceDevices contributors and is
licensed under the **GNU Affero General Public License v3.0** — see
[LICENSE](LICENSE) and [THIRD_PARTY.md](THIRD_PARTY.md). Upstream board-porting
documentation lives in [boards/README.md](boards/README.md); the custom board
follows that template.

The original passive `ESP32-SEC` prototype exists in this repository's git
history for reference only and is not part of the current build.
