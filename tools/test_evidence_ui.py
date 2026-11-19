"""Release regression gates for the USB audit evidence path."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
MAIN=ROOT/"firmware"/"main"
UI=(MAIN/"ui.c").read_text(encoding="utf-8")
REPORT=(MAIN/"sec_report.c").read_text(encoding="utf-8")
CMAKE=(MAIN/"CMakeLists.txt").read_text(encoding="utf-8")

class EvidenceUITests(unittest.TestCase):
    def test_export_only_after_scan_success(self):
        self.assertIn("sec_report_wifi(results, ap_count)", UI)
        self.assertIn("sec_report_ble(ble_results, ble_count)", UI)
        self.assertIn("scan_ready && !scan_busy && scan_result == ESP_OK",UI)
        self.assertIn("ble_ready && !ble_busy && ble_result == ESP_OK",UI)

    def test_no_unprompted_export(self):
        # Normal rendering/worker functions must never send reports automatically.
        for method in ("static void render_wifi(void)", "static void render_ble(void)",
                       "static void wifi_worker(void", "static void ble_worker(void"):
            start=UI.index(method)
            end=UI.find("\nstatic ",start+7)
            body=UI[start:] if end<0 else UI[start:end]
            self.assertNotIn("sec_report_wifi(",body,method)
            self.assertNotIn("sec_report_ble(",body,method)

    def test_footer_fits_240_pixel_screen(self):
        self.assertIn("lv_obj_set_size(start_btn, 72, 37)", UI)
        self.assertIn("lv_obj_set_size(back_btn, 72, 37)", UI)
        self.assertIn("lv_obj_set_size(lang_btn, 72, 37)", UI)
        self.assertIn("LV_ALIGN_BOTTOM_MID, 0, -10", UI)

    def test_bounded_export_and_compiled(self):
        self.assertIn("count > SEC_MAX_APS", REPORT)
        self.assertIn("count > SEC_MAX_BLE", REPORT)
        self.assertIn('SEC1|REPORT|', REPORT)
        self.assertIn('"sec_report.c"', CMAKE)

if __name__=="__main__": unittest.main()
