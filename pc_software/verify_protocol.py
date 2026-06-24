"""
Protocol verification — connects ESP32 serial, validates JSON uplink/downlink
"""

import serial
import json
import time
import sys

PORT = "COM4"
BAUD = 115200

print("=" * 60)
print("  Light Monitor — Protocol Verification")
print("=" * 60)

# 1. Open serial
print(f"\n[1] Opening {PORT} @ {BAUD}...")
try:
    ser = serial.Serial(PORT, BAUD, timeout=1)
    print(f"    OK!")
except Exception as e:
    print(f"    FAIL: {e}")
    sys.exit(1)

# 2. Wait for ESP32 to boot (reset triggered by serial open)
print("[2] Waiting for ESP32 boot + JSON data (14s)...")
time.sleep(14)

# 3. Collect JSON data
print("[3] Collecting uplink JSON data (target: 15 packets)...")
data_list = []
start = time.time()
buf = b""
while len(data_list) < 15 and (time.time() - start) < 20:
    if ser.in_waiting:
        buf += ser.read(ser.in_waiting)
        while b"\n" in buf:
            line, buf = buf.split(b"\n", 1)
            try:
                text = line.strip().decode("utf-8", errors="replace")
                obj = json.loads(text)
                if obj.get("type") == "data":
                    data_list.append(obj)
            except (json.JSONDecodeError, UnicodeDecodeError):
                pass
    time.sleep(0.05)

print(f"    Received {len(data_list)} JSON packets")

if len(data_list) == 0:
    print("    NO DATA! Serial RX may not be connected on this board.")
    print("    Try via TCP: python verify_tcp.py")
    ser.close()
    sys.exit(1)

# 4. Field completeness
print("\n[4] Field completeness check:")
required_fields = [
    "type", "light", "light_level", "light_level_name",
    "buzzer", "buzzer_mode",
    "th_high", "th_mid_h", "th_mid_l", "th_low",
    "wifi", "ip", "page", "ts"
]
all_ok = True
for field in required_fields:
    missing = sum(1 for d in data_list if field not in d)
    status = "OK" if missing == 0 else f"MISSING {missing}/{len(data_list)}"
    if missing > 0:
        all_ok = False
    print(f"    {field:20s} -> {status}")

if not all_ok:
    print("\n    *** FIELD CHECK FAILED! ***")
else:
    print("\n    *** ALL FIELDS PRESENT! ***")

# 5. Data sanity
print("\n[5] Data sanity:")
lights = [d["light"] for d in data_list]
min_l, max_l = min(lights), max(lights)
print(f"    Light range:  {min_l} ~ {max_l} mV")

level_names = set(d["light_level_name"] for d in data_list)
print(f"    Light levels: {level_names}")

# Check level consistency
level_ok = True
for d in data_list:
    lv = d["light"]
    if lv > d["th_high"]: exp = "VERY_BRIGHT"
    elif lv > d["th_mid_h"]: exp = "BRIGHT"
    elif lv >= d["th_mid_l"]: exp = "NORMAL"
    elif lv >= d["th_low"]: exp = "DIM"
    else: exp = "VERY_DARK"
    if d["light_level_name"] != exp:
        print(f"    FAIL: light={lv} expected {exp}, got {d['light_level_name']}")
        level_ok = False
        break

if level_ok:
    print(f"    Level calc:   CONSISTENT")

bz_on = sum(1 for d in data_list if d["buzzer"] == "on")
bz_off = sum(1 for d in data_list if d["buzzer"] == "off")
print(f"    Buzzer:       ON={bz_on}, OFF={bz_off}")

wifi_ok = any(d["wifi"] == "connected" and d["ip"] != "0.0.0.0" for d in data_list)
print(f"    WiFi:         {'CONNECTED' if wifi_ok else 'disconnected'}")
if wifi_ok:
    ips = [d["ip"] for d in data_list if d["wifi"] == "connected" and d["ip"] != "0.0.0.0"]
    if ips:
        print(f"    IP:           {ips[-1]}")

# 6. Sample JSON
print("\n[6] Sample JSON (last packet):")
print("-" * 60)
print(json.dumps(data_list[-1], indent=2, ensure_ascii=False))
print("-" * 60)

# 7. Downlink test
print("\n[7] Downlink test:")
cmd = '{"cmd":"get_status"}'
print(f"    Sending: {cmd}")
ser.write((cmd + "\r\n").encode())
time.sleep(2)

buf = b""
start = time.time()
while (time.time() - start) < 3:
    if ser.in_waiting:
        buf += ser.read(ser.in_waiting)
    time.sleep(0.1)

resp_count = 0
for line in buf.split(b"\n"):
    try:
        text = line.strip().decode("utf-8", errors="replace")
        if text and ('"type":"ack"' in text or '"type":"data"' in text):
            print(f"    Response: {text[:150]}")
            resp_count += 1
    except Exception:
        pass

if resp_count == 0:
    print("    (No serial response — downlink needs TCP/WiFi)")

ser.close()

# Summary
print("\n" + "=" * 60)
print("  VERIFICATION SUMMARY")
print("=" * 60)
print(f"  JSON Uplink:    {'PASS' if len(data_list) > 0 else 'FAIL'}")
print(f"  All Fields:     {'PASS' if all_ok else 'FAIL'}")
print(f"  Data Sanity:    {'PASS' if level_ok else 'FAIL'}")
print(f"  Downlink:       {'PASS' if resp_count > 0 else 'N/A (use TCP)'}")
print(f"\n  Protocol verification complete!")
