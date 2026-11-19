# ESP32-SEC RC hardware acceptance gates

The CI compile is **not** proof of boot. Version v0.2.1 is a **recovery alpha**, not a validated release candidate.

To promote a specific binary/commit to RC, a real ESP32-S3 N16R8 must pass **all eight gates** with a tester, board ID and serial log evidence:

1. LCD stays on with a legible LVGL UI through a full reboot; no blank/white loop.
2. XPT2046 five-point touch calibration and menu actions match positions.
3. Force two incomplete boots in the test lab; next boot shows recovery UI; reboot restores regular UI.
4. Wi-Fi scan finds a known test AP; audit labels the configured encryption accurately.
5. BLE scan discovers a known test peripheral; results refresh and cancel cleanly.
6. USB serial lab control is tested with demo mode then a permitted localhost-only target. Verify request vs ACK separately.
7. Device remains responsive for 30 minutes, including several scan cycles; no watchdog or heap regression.
8. Known-good rollback image restores a bootable LCD; verify manifest and merged image checksums.

The RC candidate must be the **exact commit** that passed hardware verification. Record results in JSON:
```json
{
  "commit": "<40-char-commit-SHA>",
  "board_id": "ESP32-S3 N16R8 / serial identifier",
  "tester": "test operator",
  "serial_log_reference": "evidence URL or internal record",
  "checks": {
    "lcd_persistent_ui": true,
    "touch_five_point": true,
    "boot_recovery": true,
    "wifi_scan_real": true,
    "ble_scan_real": true,
    "lab_console_loopback": true,
    "reboot_watchdog_30min": true,
    "firmware_rollback": true
  }
}
```
Run `python3 tools/check_rc_gate.py --report validation.json --commit <SHA>`.

**Never prefill `true` for an unobserved check.** GitHub Actions cannot prove physical validation. This manual gate is intentional.
