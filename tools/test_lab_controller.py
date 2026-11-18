import unittest
from lab_controller import parse_request, handle


class LabControllerTests(unittest.TestCase):
    def test_whitelist(self):
        self.assertEqual(parse_request("SEC1|CONTROL|LED_ON"), "on")
        self.assertEqual(parse_request("SEC1|CONTROL|LED_OFF\n"), "off")
        self.assertIsNone(parse_request("SEC1|CONTROL|SHELL_EXEC"))
        self.assertIsNone(parse_request("SEC1|CONTROL|LED_ON;rm -rf /"))
        self.assertIsNone(parse_request("noise SEC1|CONTROL|LED_ON"))

    def test_disabled_by_default(self):
        self.assertEqual(handle("SEC1|CONTROL|LED_ON", False)["status"], "DENIED")

    def test_demo_does_not_control_real_device(self):
        result = handle("SEC1|CONTROL|LED_OFF", True)
        self.assertEqual(result["status"], "DEMO_ONLY")
        self.assertEqual(result["led"], "off")


if __name__ == "__main__":
    unittest.main()
