#!/usr/bin/env python3
"""Hard gate for RC promotion; CI success alone cannot pass hardware criteria."""
import argparse
import json
from pathlib import Path
import sys

REQUIRED = (
    "lcd_persistent_ui",
    "touch_five_point",
    "boot_recovery",
    "wifi_scan_real",
    "ble_scan_real",
    "lab_console_loopback",
    "reboot_watchdog_30min",
    "firmware_rollback",
)

def validate(doc, expected_commit):
    errors = []
    commit = doc.get("commit", "")
    if not isinstance(commit, str) or len(commit) != 40 or commit.lower() != expected_commit.lower():
        errors.append("hardware validation commit does not match tested build SHA")
    if not doc.get("board_id"):
        errors.append("missing board identification")
    if not doc.get("tester"):
        errors.append("missing tester")
    if not doc.get("serial_log_reference"):
        errors.append("missing boot / test evidence reference")
    checks = doc.get("checks", {})
    for name in REQUIRED:
        if checks.get(name) is not True:
            errors.append("hardware gate not passed: " + name)
    return errors

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--report", type=Path, required=True)
    p.add_argument("--commit", required=True)
    opts = p.parse_args()
    doc = json.loads(opts.report.read_text(encoding="utf-8"))
    errors = validate(doc, opts.commit)
    if errors:
        for error in errors:
            print("RC BLOCKED:", error, file=sys.stderr)
        return 1
    print("Hardware RC gate PASS: all 8 checks")
    return 0

if __name__ == "__main__":
    sys.exit(main())
