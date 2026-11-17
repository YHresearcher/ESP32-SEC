# ESP32-DIV -> ESP32-SEC integration matrix

Source: https://github.com/cifertech/ESP32-DIV (MIT). Inventory based on upstream README and project source as read on 2026-11-17.

**Critical distinction:** a planned or identified feature is NOT a working module. No hardware capability is invented; unsupported buttons must remain disabled with an explicit reason. ESP32-DIV primarily uses Arduino C++ and an ILI9341 display, while ESP32-SEC is ESP-IDF 6 + LVGL 9 + ST7789V/XPT2046. Blindly copying Arduino firmware or its GPIO definitions will break the existing board.

Status vocabulary:
- `CORE_IN_PROGRESS`: implementation exists or is being connected; not validated on a physical device.
- `REQUIRES_HARDWARE`: needs a peripheral not confirmed on this ESP32-SEC board.
- `BACKLOG`: eligible functionality not yet implemented.
- `EXCLUDED`: intentionally not implemented as a disruptive/deceptive offensive function.

| Group | ESP32-DIV original capability | ESP32-SEC status | Requirements / adaptation |
|---|---|---|---|
| Wi-Fi | Wi-Fi Scanner | CORE_IN_PROGRESS | ESP32-S3; scanner already exists; hardware smoke test pending |
| Wi-Fi | Packet Monitor / channel waterfall / PCAP | BACKLOG | Promiscuous receive-only, SD for PCAP |
| Wi-Fi | Deauth Detector | BACKLOG | Passive monitor / suspicious frame counts only |
| Wi-Fi | WPS Scanner | BACKLOG | Read beacon/probe info; no PIN attack |
| Wi-Fi | ARP Scanner | BACKLOG | Only joined/authorized LAN |
| Wi-Fi | Hidden SSID Revealer | EXCLUDED | Force-disclosure mechanism; passive display of hidden SSID metadata is okay |
| Wi-Fi | Beacon Spammer | EXCLUDED | Bulk deceptive advertisement |
| Wi-Fi | Deauth Attack | EXCLUDED | Deliberate connectivity disruption |
| Wi-Fi | Captive Portal | BACKLOG | Local provisioning portal only; no cloned SSIDs or credential collection |
| Wi-Fi | Probe Flood | EXCLUDED | Network stress/flood |
| Wi-Fi | Karma Attack | EXCLUDED | Impersonation of saved networks |
| BLE | BLE Scanner | CORE_IN_PROGRESS | ESP32-S3 NimBLE; UI wiring and host sync fixes committed |
| BLE | BLE Sniffer | BACKLOG | Passive advertisement observations; MAC/RSSI/last-seen/count |
| BLE | AirTag Sniffer | BACKLOG | Identify supported advertisement signatures without tracking individuals |
| BLE | Skimmer Detect | BACKLOG | Passive heuristic; do not claim definitive detection |
| BLE | BLE Spoofer | EXCLUDED | Identity spoofing |
| BLE | Sour Apple | EXCLUDED | Unsolicited popup/notification spam |
| BLE | BLE Jammer | EXCLUDED | Radio disruption |
| BLE | BLE Rubber Ducky | EXCLUDED | Unsolicited keyboard/keystroke injection |
| BLE | AirTag Spoofer | EXCLUDED | Tracker identity spoofing |
| NRF24 | 2.4GHz Scanner | REQUIRES_HARDWARE | NRF24L01+; later receive-only monitor |
| NRF24 | ESB Sniffer | REQUIRES_HARDWARE | NRF24L01+; passive receive |
| NRF24 | MouseJack Scan | REQUIRES_HARDWARE | NRF24; authorized detection only |
| NRF24 | ESB Replay | EXCLUDED | Packet replay to third-party receivers |
| NRF24 | Protokill | EXCLUDED | Interference/disruption |
| NRF24 | MouseJack Inject | EXCLUDED | Wireless keystroke injection |
| Sub-GHz | Saved Profiles | REQUIRES_HARDWARE | CC1101 receiver + SD; receive-only stored captures |
| Sub-GHz | Jamming Detector | REQUIRES_HARDWARE | CC1101; passive spectrum/energy detection |
| Sub-GHz | Replay Attack | EXCLUDED | Access-control command replay |
| Sub-GHz | Sub-GHz Jammer | EXCLUDED | Radio disruption |
| Sub-GHz | De Bruijn / Brute Force | EXCLUDED | Access-control brute force |
| IR | IR receive/visualization/storage | REQUIRES_HARDWARE | IR receiver; user-owned remotes |
| IR | Saved Profiles | REQUIRES_HARDWARE | IR receiver + microSD |
| IR | Universal IR Controller | REQUIRES_HARDWARE | IR LED/transmitter; local equipment control |
| RFID/NFC | Card Reader | REQUIRES_HARDWARE | RFID/NFC reader; UID + tag type |
| RFID/NFC | Dump | REQUIRES_HARDWARE | Authorized tags and known keys |
| RFID/NFC | Decode Access | REQUIRES_HARDWARE | Interpret access rules for owned tags |
| RFID/NFC | Card Clone | EXCLUDED | Credential duplication |
| RFID/NFC | Erase | EXCLUDED | Destructive modification of access tags |
| RFID/NFC | Jam Reader | EXCLUDED | RF interference |
| RFID/NFC | Tag Disrupt | EXCLUDED | RF interference |
| RFID/NFC | Disrupt Emulate | EXCLUDED | RF interference |
| GPS | Wardriver | REQUIRES_HARDWARE | GNSS + microSD, opt-in location logging |
| GPS | Satellite Scanner | REQUIRES_HARDWARE | GNSS fix and sky-view details |
| System | Serial Monitor | BACKLOG | On-screen debug receive and ring buffer |
| System | SD File Manager | REQUIRES_HARDWARE | MicroSD slot and pin/voltage definition |
| System | Update Firmware | REQUIRES_HARDWARE | SD and verified firmware signature/integrity |
| System | Touch Calibrate | BACKLOG | 4-corner calibration + persistent NVS |
| System | Settings | CORE_IN_PROGRESS | Language persistence exists; brightness depends on backlight wiring |

## Hardware detection / integration gates

1. Never use ESP32-DIV GPIOs, ILI9341 configuration, or drivers on this ST7789V board.
2. Keep Wi-Fi/BLE scan features only while `ESP32-S3` is available; prefer ESP-IDF/NimBLE drivers.
3. CC1101, NRF24, RFID/NFC, IR, GNSS and SD features are **not installed** until driver, pins, CS conflicts, bus speed and voltage are confirmed. They must not claim to run.
4. Preserve existing BLE and Wi-Fi modules when importing; no duplicate competing driver init.
5. Confirm actual hardware behavior; CI compile success is not hardware validation.
6. Include original ESP32-DIV MIT attribution when reusing third-party source. No bulk copy has yet been performed.
7. Avoid fake indicators: no hardcoded battery %/wall-clock in UI without measured battery supply or RTC/network time.

## Immediate integration priority

- Fix BLE screen/result wiring and NimBLE host synchronization.
- Font with all Vietnamese diacritics and proper LVGL glyph coverage; preserve VI/EN.
- Replace old LVGL home screen with compact 240x320 screen inspired by approved mockup; no oversized raster screenshot.
- Implement passive Wi-Fi packet monitor and BLE advertisement metrics.
- Add peripherals one at a time: NRF24, CC1101, IR, RFID/NFC, GNSS, SD.
