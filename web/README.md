# ESP32-SEC web installer

Static HTML/JS app. Deploy the `web/` directory to GitHub Pages or Cloudflare Pages with HTTPS.

**Safety gate:** `manifest.json` is intentionally absent. Installer stays disabled until a firmware release is manually hardware-validated. CI artifacts alone do not authorize flashing.

For an approved release, copy `manifest.example.json` to `manifest.json`, replace all placeholder URLs and check the offsets/build profile. The sample is **not valid for production flashing**.

The Web Serial console requests permission through a user gesture; requires compatible browser and USB driver.
