import unittest
from check_rc_gate import REQUIRED, validate

COMMIT = "a" * 40
GOOD = {
    "commit": COMMIT, "board_id": "ESP32-S3 N16R8",
    "tester": "hardware test operator",
    "serial_log_reference": "test-run-1",
    "checks": {x: True for x in REQUIRED},
}

class RCGateTests(unittest.TestCase):
    def test_good(self):
        self.assertEqual(validate(GOOD, COMMIT), [])

    def test_fails_without_hardware_evidence(self):
        self.assertTrue(validate({}, COMMIT))

    def test_fails_different_commit(self):
        self.assertTrue(validate(GOOD, "b" * 40))

    def test_fails_each_required_check(self):
        for name in REQUIRED:
            bad = {**GOOD, "checks": {**GOOD["checks"], name: False}}
            self.assertTrue(validate(bad, COMMIT), name)

if __name__ == "__main__":
    unittest.main()
