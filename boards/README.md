# Boards in this fork

This fork carries **one** board: `esp32-sec-chibi` (ESP32-S3-WROOM-1-N16R8,
ST7789V 240x320, XPT2046). It is the `default_envs` target in
`platformio.ini`, so `pio run` builds only this device.

`_jsonfiles/` is not a board — it holds the generic MCU pin definitions and
board JSON that PlatformIO needs for `board = esp32s3`.

All other upstream Bruce board ports were removed to keep the tree focused.
The upstream porting guide (including the `_New-Device-Model` template) is at
<https://github.com/BruceDevices/firmware/blob/main/boards/README.md>.
