"""Regression guard for ESP32-SEC LVGL home menu initialization."""
from pathlib import Path
import re
import unittest

SOURCE = Path(__file__).resolve().parents[1] / "firmware" / "main" / "ui.c"


class UiMenuTests(unittest.TestCase):
    def test_tile_initialization_matches_refresh(self):
        src = SOURCE.read_text(encoding="utf-8")
        declared = int(re.search(r"tiles\[(\d+)\]", src).group(1))
        loops = re.findall(r"for \(int i=0;i<(\d+);i\+\+\)", src)
        self.assertGreaterEqual(len(loops), 2)
        self.assertEqual([declared] * 2, list(map(int, loops[:2])),
                         "UI must initialize all menu tiles before refreshing them")

    def test_guard_missing_tile(self):
        src = SOURCE.read_text(encoding="utf-8")
        self.assertIn("if (!tiles[i]) continue;", src)


if __name__ == "__main__":
    unittest.main()
