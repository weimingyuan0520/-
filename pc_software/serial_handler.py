"""
serial_handler.py — 串口通信管理 (后台线程)
"""

import serial
import serial.tools.list_ports
import threading
import time
from protocol import parse_uplink


class SerialHandler:
    def __init__(self, on_data=None, on_status=None):
        self.port = None
        self.ser = None
        self.running = False
        self.thread = None
        self.on_data = on_data      # 回调: on_data(parsed_dict)
        self.on_status = on_status  # 回调: on_status("Connected"|"Disconnected"|...)

    @staticmethod
    def list_ports():
        """枚举可用串口"""
        ports = serial.tools.list_ports.comports()
        return [p.device for p in ports]

    def connect(self, port, baudrate=115200):
        if self.running:
            self.disconnect()
        try:
            self.ser = serial.Serial(port, baudrate, timeout=0.5)
            self.port = port
            self.running = True
            self.thread = threading.Thread(target=self._read_loop, daemon=True)
            self.thread.start()
            if self.on_status:
                self.on_status("Connected", f"Serial {port} @ {baudrate}")
            return True
        except serial.SerialException as e:
            if self.on_status:
                self.on_status("Error", str(e))
            return False

    def disconnect(self):
        self.running = False
        if self.thread:
            self.thread.join(timeout=1)
        if self.ser and self.ser.is_open:
            self.ser.close()
        self.ser = None
        self.port = None
        if self.on_status:
            self.on_status("Disconnected", "")

    def send(self, data_str):
        if self.ser and self.ser.is_open:
            try:
                self.ser.write((data_str + "\r\n").encode("utf-8"))
                return True
            except serial.SerialException:
                pass
        return False

    def _read_loop(self):
        buf = b""
        while self.running:
            try:
                if self.ser.in_waiting:
                    chunk = self.ser.read(self.ser.in_waiting)
                    buf += chunk
                    # 按换行分割
                    while b"\n" in buf:
                        line, buf = buf.split(b"\n", 1)
                        line = line.strip()
                        if line:
                            try:
                                text = line.decode("utf-8", errors="replace")
                                data = parse_uplink(text)
                                if data and self.on_data:
                                    self.on_data(data)
                            except Exception:
                                pass
            except (serial.SerialException, OSError):
                if self.running and self.on_status:
                    self.on_status("Disconnected", "Serial port lost")
                self.running = False
                break
            time.sleep(0.02)

    def is_connected(self):
        return self.running and self.ser and self.ser.is_open
