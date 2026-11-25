# ESP32-SEC website

Static GitHub Pages site for the Bruce-based ESP32-S3 Chibi firmware.

`index.html` is an informational landing page with an EN/VI toggle. It does
**not** offer a web installer: no release manifest or verified binary has been
published yet, and the old ESP32-SEC alpha installer workflows were removed
during the pivot to Bruce.

To deploy, enable GitHub Pages with **Source: GitHub Actions** in repository
Settings, or publish `web/` as a static site by any other means.

When a hardware-verified release exists, an `esp-web-tools` manifest can be
added back. Do not publish a flashing manifest before bootloader offsets,
partitions and a recovery procedure have been verified on real hardware.
