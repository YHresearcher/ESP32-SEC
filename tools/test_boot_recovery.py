"""Static regression gates for early-boot recovery and LCD initialization.
These tests do NOT replace real hardware validation.
"""
from pathlib import Path
import unittest

BASE = Path(__file__).resolve().parents[1] / "firmware" / "main"


class BootRecoveryTests(unittest.TestCase):
    def test_guard_called_before_display(self):
        text = (BASE / "main.c").read_text()
        self.assertLess(text.index("sec_boot_guard_begin("), text.index("sec_board_init("))

    def test_recovery_never_initializes_radios(self):
        text = (BASE / "main.c").read_text()
        self.assertLess(text.index('if (recovery_mode) {\n        ESP_LOGW(TAG, "SEC_RECOVERY_ACTIVE'), text.index("sec_wifi_init()"))
        self.assertIn("sec_board_init(&display, recovery_mode)", text)

    def test_no_boot_white_dma_probe(self):
        text = (BASE / "board.c").read_text()
        self.assertNotIn("esp_lcd_panel_draw_bitmap(panel", text)
        self.assertNotIn("s_lcd_probe", text)
        self.assertIn("if (recovery_mode)", text)

    def test_minimal_recovery_ui_and_heartbeat(self):
        text = (BASE / "ui.c").read_text()
        self.assertIn('RECOVERY_UI_READY', text)
        self.assertIn('sec_boot_guard_healthy()', text)
        self.assertIn("sec_ui_init(bool recovery_mode)", text)

    def test_boot_guard_nvs_recovery_after_two(self):
        text = (BASE / "boot_guard.c").read_text()
        self.assertIn("failures >= 2", text)
        self.assertIn('nvs_commit(handle)', text)
        self.assertIn('nvs_set_u8(handle, KEY, 0)', text)

if __name__ == "__main__":
    unittest.main()
