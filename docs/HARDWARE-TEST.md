# ESP32-SEC hardware verification procedure (ESP32-S3 N16R8)

**Do this only once GitHub Actions firmware build is green.** Source code is currently a development prototype; avoid replacing your working Chibi build before backups are complete.

## 0. Preparation
- Confirm the module reads ESP32-S3-WROOM N16R8 and uses the same wiring as working Chibi.
- Mac connected with a USB data cable.
- Know how to enter ROM download mode: hold BOOT, tap RESET, release BOOT.
- Keep original Chibi image and settings in a private location. Full flash dumps can contain tokens and Wi-Fi credentials; **never upload a backup to a public issue/repository**.
- Close serial monitors before flashing.

## 1. Back up the existing firmware
Using a matching version of esptool installed on Mac:
```sh
python3 -m esptool --port /dev/cu.usbmodemXXXX flash-id
python3 -m esptool --port /dev/cu.usbmodemXXXX read-flash 0x0 0x1000000 chibi-backup-16mb.bin
```
Replace port with the device path. Verify the output file is **16,777,216 bytes**; keep it private. Only use 0x1000000 size if flash-id has verified 16 MB.

## 2. Compile and validate
```sh
cd firmware
idf.py set-target esp32s3
idf.py build
```
Check the build's partition table, binary outputs and actual flash offsets. Restore Chibi if ESP32-SEC does not initialize. **Do not flash using the example website manifest.**

## 3. Smoke test
- [ ] Board boots without repeated reset.
- [ ] Chibi LCD pinout retained; normal red/green/blue, orientation correct.
- [ ] XPT2046 5-point tap accuracy and no false touches.
- [ ] Buttons in all six menus respond; Back works.
- [ ] Wi-Fi scanner discovers authorized access points, RSSI/channel plausible.
- [ ] BLE passive scan shows nearby advertisements without connecting.
- [ ] Analytics reflects previous Wi-Fi scan.
- [ ] Language selection persists after power cycle (ASCII VI fallback is a known limitation).
- [ ] No noticeable freezes/memory leaks for 60 minutes.
- [ ] BOOT+RESET ROM mode remains reachable for recovery.

## 4. Report results
Record firmware commit SHA, hardware photo/part number, GPIOs, ESP-IDF version, serial boot logs (redact credentials), which tests passed/failed and whether restored Chibi. CI pass alone does not complete this checklist.
