#!/usr/bin/env python3
"""Capture ESP32-SEC boot logs and flag crash/recovery clues.

This is an observation tool. Successful serial logs do NOT prove that LCD,
touch, BLE or physical IO work.
"""
import argparse
import json
import re
import time
from pathlib import Path

STAGES = (
    "SEC_BOOT_STAGE_1", "SEC_BOOT_STAGE_2", "SEC_BOOT_STAGE_3",
    "SEC_LCD_STAGE_08", "SEC_LCD_STAGE_14", "SEC_BOOT_STAGE_4",
    "SEC_BOOT_STAGE_5", "SEC_BOOT_STAGE_6", "SEC_BOOT_READY",
)
FAULTS = ("Guru Meditation Error", "Task watchdog", "abort()",
          "SEC_LCD_INIT_FAILED", "Recovery LVGL mutex timeout")


def analyze(text):
    present = [x for x in STAGES if x in text]
    faults = [x for x in FAULTS if x.lower() in text.lower()]
    boot_count = len(re.findall("SEC_BOOT_STAGE_1", text))
    recovery = "RECOVERY_UI_READY" in text
    return {
        "observed_stages": present,
        "fault_indicators": faults,
        "boot_stage_1_count": boot_count,
        "recovery_ui_reported": recovery,
        "boot_ready_reported": "SEC_BOOT_READY" in text,
        "hardware_lcd_validated": False,
        "hardware_touch_validated": False,
        "note": "These are serial-only observations; physical UI checks remain mandatory."
    }


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--port", help="USB serial port; omit when inspecting --log")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--seconds", type=int, default=45)
    p.add_argument("--log", type=Path, help="Analyze an existing log instead of serial capture")
    p.add_argument("--output", type=Path, default=Path("esp32-sec-boot-log.txt"))
    args = p.parse_args()
    if args.log:
        captured = args.log.read_text(encoding="utf-8", errors="replace")
    else:
        if not args.port:
            p.error("Provide --port or --log")
        try:
            import serial
        except ImportError as exc:
            raise SystemExit("Install pyserial: python -m pip install pyserial") from exc
        chunks = []
        with serial.Serial(args.port, args.baud, timeout=0.5) as port:
            end = time.monotonic() + max(1, min(args.seconds, 600))
            while time.monotonic() < end:
                raw = port.readline()
                if raw:
                    line = raw.decode("utf-8", "replace")
                    chunks.append(line)
                    print(line, end="", flush=True)
        captured = "".join(chunks)
        args.output.write_text(captured, encoding="utf-8")
    summary = analyze(captured)
    target = args.output.with_suffix(".summary.json")
    target.write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
