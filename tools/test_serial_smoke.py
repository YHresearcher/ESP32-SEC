import unittest
from serial_smoke import analyze

class SerialSmokeTests(unittest.TestCase):
    def test_detects_startup_and_fault(self):
        result=analyze("SEC_BOOT_STAGE_1\nSEC_LCD_STAGE_14\nGuru Meditation Error\n")
        self.assertEqual(result["boot_stage_1_count"], 1)
        self.assertIn("Guru Meditation Error",result["fault_indicators"])
        self.assertFalse(result["hardware_lcd_validated"])
    def test_marks_only_serial_observations(self):
        result=analyze("SEC_BOOT_READY\nRECOVERY_UI_READY\n")
        self.assertTrue(result["boot_ready_reported"])
        self.assertTrue(result["recovery_ui_reported"])
        self.assertFalse(result["hardware_touch_validated"])
if __name__=="__main__":
    unittest.main()
