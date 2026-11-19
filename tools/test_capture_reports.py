import unittest
from capture_reports import parse_report_line

class ReportCaptureTests(unittest.TestCase):
    def test_wifi(self):
        s='SEC1|REPORT|{"type":"wifi","count":1,"aps":[{"ssid":"office","channel":6}]}'
        self.assertEqual(parse_report_line(s)["count"],1)
    def test_ble(self):
        s='SEC1|REPORT|{"type":"ble","count":0,"devices":[]}'
        self.assertEqual(parse_report_line(s)["type"],"ble")
    def test_bogus_or_truncated_rejected(self):
        for s in ('SEC1|REPORT|{"type":"wifi","count":2,"aps":[]}',
                  'SEC1|REPORT|{"type":"wifi","count":1,',
                  'SEC1|REPORT|{"type":"shell","count":0,"aps":[]}',
                  'SEC1|REPORT|{"type":"ble","count":100,"devices":[]}',
                  'junk', 'SEC1|REPORT|'+'a'*20001):
            self.assertIsNone(parse_report_line(s),s[:80])

if __name__=="__main__": unittest.main()
