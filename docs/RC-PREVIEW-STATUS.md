# ESP32-SEC RC preview — feature and validation notes

The high-contrast LCD, Wi-Fi passive configuration audit, BLE passive discovery,
Recovery Mode, and bounded local parser self-test are implemented.

## Operator-driven audit evidence (new)

The **Wi-Fi**, **Phòng vệ/Defense**, **Phân tích/Analytics**, and **BLE** pages
enable a **Xuất JSON / JSON** action after a successful scan.
The button sends exactly one line beginning `SEC1|REPORT|` to the configured
USB/UART console. It includes nearby network identifiers; treat the resulting
report as private audit evidence. Nothing uploads automatically.

To capture reports on a PC:

```shell
python -m pip install pyserial
python tools/capture_reports.py --port COM7 --output audit-evidence.jsonl
```

Use the actual serial port (on macOS something like /dev/cu.usbmodem...).
Press the JSON button **only after** the serial capture is running.
Each successful Wi-Fi/BLE export appends one JSON object per line to the file.
This is **not** an exploit report or proof of compromise.

## RC status & hardware verification

Hardware board profile: ESP32-S3 N16R8; ST7789V 240x320; XPT2046.
LCD wiring unchanged, backlight GPIO not guessed, I2S pins remain untouched.

User has reported that the prior **white/blank boot issue is fixed**.
This feedback is useful but does not certify other checks. Actual physical
evidence is required for the remaining acceptance gates in
[RC-ACCEPTANCE.md](RC-ACCEPTANCE.md), particularly touch 5-point alignment,
real BLE/Wi-Fi scans, sustained operation, USB serial loopback, safe boot,
and firmware rollback. No CI workflow can certify these by itself.

The official RC workflow intentionally refuses promotion without a
commit-matched JSON hardware-validation report.

## Not implemented in this release

- External-target exploit delivery, post-exploitation control or remote shell
- BLE GATT discovery/permissions audit and raw management-frame sniffing
- USB controller confirmations / ACKs; LED requests remain request-only
- External NRF24, CC1101, NFC/IR/GNSS hardware drivers
- Automatic history persistence on device; evidence requires USB export

These capabilities are not represented as working features or counted as
passing the RC quality gate.
