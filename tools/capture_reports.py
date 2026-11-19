#!/usr/bin/env python3
"""Collect operator-requested SEC1|REPORT JSON evidence from an ESP32-SEC console.

Only reads serial data and writes JSONL locally. Does not transmit anything.
"""
import argparse
import json
from pathlib import Path
import sys

PREFIX = "SEC1|REPORT|"
MAX_LINE = 20000
VALID_TYPES = {"wifi", "ble"}

def parse_report_line(line: str):
    if not line.startswith(PREFIX) or len(line) > MAX_LINE:
        return None
    try:
        doc = json.loads(line[len(PREFIX):])
    except (ValueError, TypeError):
        return None
    if not isinstance(doc, dict) or doc.get("type") not in VALID_TYPES:
        return None
    if not isinstance(doc.get("count"), int) or not 0 <= doc["count"] <= 30:
        return None
    field = "aps" if doc["type"] == "wifi" else "devices"
    if not isinstance(doc.get(field), list) or len(doc[field]) != doc["count"]:
        return None
    return doc

def main():
    p=argparse.ArgumentParser(description="ESP32-SEC local report capture")
    p.add_argument("--port", help="Serial device, e.g. COM7 or /dev/ttyACM0")
    p.add_argument("--baud",type=int,default=115200)
    p.add_argument("--stdin",action="store_true")
    p.add_argument("--output",type=Path,default=Path("esp32-sec-evidence.jsonl"))
    args=p.parse_args()
    if not args.port and not args.stdin:
        p.error("Specify --port or --stdin")
    stream = sys.stdin
    if not args.stdin:
        try:
            import serial
        except ImportError as exc:
            raise SystemExit("Install pyserial: python -m pip install pyserial") from exc
        stream=serial.Serial(args.port,args.baud,timeout=1)
    try:
        with args.output.open("a",encoding="utf-8") as output:
            while True:
                data=stream.readline()
                if not data:
                    if args.stdin: break
                    continue
                if isinstance(data,bytes): data=data.decode("utf-8","replace")
                report=parse_report_line(data.strip())
                if report is not None:
                    output.write(json.dumps(report,ensure_ascii=False,separators=(",",":"))+"\n")
                    output.flush()
                    print("Saved",report["type"],report["count"])
    except KeyboardInterrupt:
        pass
    finally:
        if not args.stdin: stream.close()

if __name__=="__main__": main()
