"""High contrast, menu layout and security analytics must remain visible."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
UI=(ROOT/"firmware/main/ui.c").read_text(encoding="utf-8")
AUDIT=(ROOT/"firmware/main/security_audit.c").read_text(encoding="utf-8")
REG=(ROOT/"firmware/main/feature_registry.c").read_text(encoding="utf-8")

class ContrastAndAuditTests(unittest.TestCase):
    def test_explicit_white_labels(self):
        for token in (
            "lv_obj_set_style_text_color(body, lv_color_hex(SEC_TEXT_WHITE)",
            "lv_obj_set_style_text_color(label, lv_color_hex(SEC_TEXT_WHITE)",
            "lv_obj_set_style_text_color(bl, lv_color_hex(SEC_TEXT_WHITE)",
            "lv_obj_set_style_text_color(sl, lv_color_hex(SEC_TEXT_WHITE)",
            "lv_obj_set_style_text_color(ll, lv_color_hex(SEC_TEXT_WHITE)",
            "lv_obj_set_style_text_color(recovery_label, lv_color_hex(SEC_TEXT_WHITE)",
        ):
            self.assertIn(token,UI,token)
    def test_nine_tiles_clear_bottom_controls(self):
        self.assertIn("lv_obj_set_size(tiles[i], 106, 40)", UI)
        self.assertIn("39+(i/2)*44", UI)
        # Fifth row ends at y=255; bottom button begins approximately y=273.
        self.assertLess(39+(8//2)*44+40, 273)
    def test_scan_produces_real_metrics_not_placeholder(self):
        for token in ("a.high_risk", "sec_wifi_auth_name", "a.channels[1]",
                      "a.moderate", "sec_ble_audit(ble_results", "sec_wifi_audit(results"):
            self.assertIn(token, UI)
        self.assertIn("++out->high_risk", AUDIT)
        self.assertIn("++out->moderate", AUDIT)
    def test_registry_not_overstating_gatt(self):
        self.assertIn('"BLE_SIGNAL_AUDIT"', REG)
        self.assertNotIn('"BLE_GATT_AUDIT"', REG)

if __name__=="__main__":
    unittest.main()
