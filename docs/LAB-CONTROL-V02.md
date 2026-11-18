# ESP32-SEC Lab Control v0.2 — integration and hardware safety

## Source of hardware facts
The user-provided equipment document names **ModESP32 / Chibi AI / XiaoZhi**. This is a distinct project from **ESP32-SEC**. The pin mapping below is used as a hardware safety constraint only. Do not merge the two firmware applications or assume their audio features are already part of ESP32-SEC.

Confirmed by supplied equipment description:
- ESP32-S3 N16R8, ST7789V 240x320 RGB565, XPT2046 resistive touchscreen.
- LCD/touch share SPI2: SCK GPIO12, MOSI GPIO11, MISO GPIO8; LCD CS10/DC13/RST14, touch CS9/IRQ7.
- I2S microphone BCLK4, WS5, data6.
- I2S MAX98357A amplifier BCLK15, LRCLK16, data17.
- BOOT GPIO0. LCD backlight GPIO is not verified; **never assign GPIO21 by guess**.
- XPT2046 calibration, speaker output, exact power rails are not independently verified.

This integration changes **no board GPIO assignments**. It adds no external radio transmission, shell command, arbitrary target selection or remote-control takeover capability.

## Implemented firmware capabilities

1. Wi-Fi scan / configuration audit / 2.4 GHz channel counts.
2. BLE passive scan / RSSI summary.
3. Pentest Lab: local bounded parser self-test (five cases); this is not a network exploit.
4. Lab Control screen: user manually taps `LED ON` or `LED OFF`. Firmware emits exactly `SEC1|CONTROL|LED_ON` or `SEC1|CONTROL|LED_OFF` over its **configured console**. Sending a line is **not an acknowledgement of target execution**.
5. Module page indicates missing NRF24, CC1101, IR, NFC, GNSS and microSD hardware.

## PC controller (Python)

The PC is the only place that may apply the control action. It is disabled by default, checks an exact allowlist, and can be used with a purely simulated local LED state.

```shell
cd tools
python -m unittest -v test_lab_controller.py
python lab_controller.py --stdin --enable-control
# Type: SEC1|CONTROL|LED_ON
# -> DEMO_ONLY
```

For a USB serial port, install `pyserial` and explicitly opt in:

```shell
python -m pip install pyserial
python tools/lab_controller.py --port COM7 --baud 115200 --enable-control
```

On macOS/Linux substitute the actual serial device for COM7. USB serial availability depends on the specific board's USB/USB-UART wiring and ESP-IDF console configuration; `printf` may be routed to a different port from the programming connection. Verify via an independent serial monitor first.

By default, the controller only simulates a LED, not a real physical device. For **an explicitly configured local lab service only**, use:

```shell
SEC_LAB_TOKEN=your_local_test_token python tools/lab_controller.py --port /dev/ttyACM0 --enable-control --loopback-http
```

The gateway makes only POST requests to `http://127.0.0.1:8765/api/led` with a JSON payload `{"state":"on"}` or `{"state":"off"}`. It cannot choose arbitrary URLs, run shell commands, or operate on third-party devices. You must provide your own local, authorized LED lab service implementing that endpoint.

## Verification gates

- GitHub Actions compilation and Python unit tests.
- A USB console monitor visibly shows exactly one `SEC1|CONTROL|LED_ON` per tap.
- Demo host reports `DEMO_ONLY` and never claims a physical LED switched.
- Loopback mode is opt-in, with target explicitly running on 127.0.0.1:8765; confirm response on the host console.
- Verify all touchscreen buttons and the LVGL home screen on physical 240x320 hardware.
- Validate Wi-Fi, BLE coexistence and memory usage on real hardware.
- Preserve known-good firmware rollback; do not erase a working ModESP32 image without a backup.

**Security boundary:** this project supports lab validation and owned-device control only. No general remote shell, persistence, arbitrary exploit delivery, radio jamming, unauthorized control, or credential collection is implemented.
