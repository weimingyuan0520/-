"""
tcp_handler.py — TCP Socket 通信管理 (后台线程)
"""

import socket
import threading
import time
from protocol import parse_uplink


class TcpHandler:
    def __init__(self, on_data=None, on_status=None):
        self.sock = None
        self.running = False
        self.thread = None
        self.host = ""
        self.port = 8080
        self.on_data = on_data
        self.on_status = on_status

    def connect(self, host, port=8080):
        if self.running:
            self.disconnect()
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(3)
            self.sock.connect((host, port))
            self.sock.settimeout(0.5)
            self.host = host
            self.port = port
            self.running = True
            self.thread = threading.Thread(target=self._read_loop, daemon=True)
            self.thread.start()
            if self.on_status:
                self.on_status("Connected", f"TCP {host}:{port}")
            return True
        except (socket.error, OSError) as e:
            if self.on_status:
                self.on_status("Error", str(e))
            return False

    def disconnect(self):
        self.running = False
        if self.thread:
            self.thread.join(timeout=1)
        if self.sock:
            try:
                self.sock.close()
            except Exception:
                pass
        self.sock = None
        if self.on_status:
            self.on_status("Disconnected", "")

    def send(self, data_str):
        if self.sock and self.running:
            try:
                self.sock.sendall((data_str + "\r\n").encode("utf-8"))
                return True
            except (socket.error, OSError):
                pass
        return False

    def _read_loop(self):
        buf = b""
        while self.running:
            try:
                chunk = self.sock.recv(4096)
                if not chunk:
                    if self.running and self.on_status:
                        self.on_status("Disconnected", "Server closed connection")
                    self.running = False
                    break
                buf += chunk
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
            except socket.timeout:
                continue
            except (socket.error, OSError):
                if self.running and self.on_status:
                    self.on_status("Disconnected", "Connection lost")
                self.running = False
                break
            time.sleep(0.02)

    def is_connected(self):
        return self.running and self.sock is not None
