#!/usr/bin/env python3
"""ESP32-SEC allowlisted lab controller. Python 3.10+, pyserial for USB use.

Nothing executes unless --enable-control is present.
Only the literal SEC1|CONTROL|LED_ON/OFF messages are accepted.
Demo mode changes a local boolean; --loopback-http sends POST to 127.0.0.1 only.
"""
import argparse
import json
import os
import sys
import urllib.request
from datetime import datetime, timezone

PREFIX = "SEC1|CONTROL|"
ACTIONS = {"LED_ON": "on", "LED_OFF": "off"}


def parse_request(line: str):
    raw = line.strip()
    if not raw.startswith(PREFIX):
        return None
    action = raw[len(PREFIX):]
    return ACTIONS.get(action)


def execute(state: str, *, enabled: bool, loopback_http: bool = False):
    if state not in ACTIONS.values():
        raise ValueError("Action not allowlisted")
    if not enabled:
        return {"status": "DENIED", "reason": "control_disabled"}
    if not loopback_http:
        # Deliberately no real device is touched in demo mode.
        return {"status": "DEMO_ONLY", "led": state}
    url = "http://127.0.0.1:8765/api/led"
    body = json.dumps({"state": state}).encode("utf-8")
    headers = {"Content-Type": "application/json"}
    token = os.environ.get("SEC_LAB_TOKEN")
    if token:
        headers["Authorization"] = f"Bearer {token}"
    request = urllib.request.Request(url, data=body, headers=headers, method="POST")
    with urllib.request.urlopen(request, timeout=2) as response:
        if response.status != 200:
            raise RuntimeError(f"HTTP {response.status}")
        payload = response.read(2048)
    return {"status": "TARGET_RESPONDED", "led": state, "response": payload.decode("utf-8", "replace")}


def handle(line: str, enabled: bool, loopback_http: bool = False):
    state = parse_request(line)
    if state is None:
        return None
    timestamp = datetime.now(timezone.utc).isoformat()
    try:
        result = execute(state, enabled=enabled, loopback_http=loopback_http)
    except Exception as exc:
        result = {"status": "ERROR", "reason": str(exc)[:140]}
    return {"ts": timestamp, "action": f"LED_{state.upper()}", **result}


def main():
    p = argparse.ArgumentParser(description="Allowlisted USB lab LED controller")
    p.add_argument("--port", help="Serial port (e.g. COM7 or /dev/ttyACM0)")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--enable-control", action="store_true", help="Mandatory opt-in to execute fixed actions")
    p.add_argument("--loopback-http", action="store_true", help="Forward only to local 127.0.0.1:8765")
    p.add_argument("--stdin", action="store_true", help="Test protocol from standard input")
    a = p.parse_args()
    if not a.stdin and not a.port:
        p.error("Choose --port or --stdin")
    if a.loopback_http and not a.enable_control:
        p.error("--loopback-http requires --enable-control")
    print("LAB CTRL: " + ("enabled" if a.enable_control else "read-only") +
          (" / loopback target" if a.loopback_http else " / demo target"), flush=True)
    if a.stdin:
        source = sys.stdin
    else:
        try:
            import serial
        except ImportError as exc:
            raise SystemExit("Install pyserial: python -m pip install pyserial") from exc
        source = serial.Serial(a.port, a.baud, timeout=1)
    try:
        while True:
            raw = source.readline()
            if isinstance(raw, bytes):
                line = raw.decode("utf-8", "replace")
            else:
                line = raw
            if not line:
                if a.stdin:
                    break
                continue
            result = handle(line, a.enable_control, a.loopback_http)
            if result is not None:
                print(json.dumps(result, ensure_ascii=False), flush=True)
    except KeyboardInterrupt:
        pass
    finally:
        if not a.stdin:
            source.close()


if __name__ == "__main__":
    main()
