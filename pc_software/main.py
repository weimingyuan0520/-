"""
main.py — 智能光敏监控系统 PC 上位机 (中文界面)
CustomTkinter + Matplotlib
"""

import queue
from datetime import datetime
from collections import deque

import customtkinter as ctk
import matplotlib
matplotlib.rcParams["font.family"] = "Microsoft YaHei"
matplotlib.rcParams["axes.unicode_minus"] = False
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.dates as mdates

from serial_handler import SerialHandler
from tcp_handler import TcpHandler
from protocol import (
    build_command_set_threshold,
    build_command_buzzer,
    build_command_get_status,
    validate_thresholds,
)

# ============================================================
#  Constants
# ============================================================
HISTORY_LENGTH = 120
PLOT_INTERVAL_MS = 500
LOG_MAX_LINES = 500

LIGHT_COLORS = {
    0: "#FF4444",
    1: "#FFAA00",
    2: "#44DD44",
    3: "#44AAFF",
    4: "#8844FF",
}

LIGHT_NAMES_CN = {
    "VERY_BRIGHT": "极强光",
    "BRIGHT":      "偏亮",
    "NORMAL":      "正常",
    "DIM":         "偏暗",
    "VERY_DARK":   "极暗",
}

ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("blue")

# Microsoft YaHei 中文字体
def CN(size=12, bold=False):
    return ctk.CTkFont(family="Microsoft YaHei", size=size,
                       weight="bold" if bold else "normal")


class LightMonitorApp(ctk.CTk):
    def __init__(self):
        super().__init__()

        self.title("智能光敏监控系统 — PC 上位机")
        self.geometry("1280x800")
        self.minsize(1024, 640)

        self.data_queue = queue.Queue()
        self.history = deque(maxlen=HISTORY_LENGTH)
        self.latest_data = None

        self.serial = SerialHandler(
            on_data=self._on_data, on_status=self._on_status)
        self.tcp = TcpHandler(
            on_data=self._on_data, on_status=self._on_status)

        self.log_lines = []
        self._syncing_sliders = False   # 防止滑块同步时触发回调循环
        self._sliders_synced = False    # 仅在首次收到数据时同步滑块

        self._build_ui()
        self._poll_data()
        self._update_plot()

    # ============================================================
    #  UI
    # ============================================================
    def _build_ui(self):
        self.grid_columnconfigure(0, weight=0, minsize=220)
        self.grid_columnconfigure(1, weight=3)
        self.grid_columnconfigure(2, weight=1, minsize=240)
        self.grid_rowconfigure(0, weight=3)
        self.grid_rowconfigure(1, weight=1)

        self._build_left_panel()
        self._build_center_panel()
        self._build_right_panel()
        self._build_log_panel()

    def _build_left_panel(self):
        frame = ctk.CTkFrame(self)
        frame.grid(row=0, column=0, rowspan=2, padx=8, pady=8, sticky="nsew")
        frame.grid_columnconfigure(0, weight=1)

        ctk.CTkLabel(frame, text="通信连接",
                     font=ctk.CTkFont(size=16, weight="bold")).pack(pady=(12, 8))

        # 串口
        ctk.CTkLabel(frame, text="串口 (UART)",
                     font=ctk.CTkFont(size=13, weight="bold")).pack(anchor="w", padx=12)
        ctk.CTkLabel(frame, text="选择串口端口:",
                     font=ctk.CTkFont(size=11)).pack(anchor="w", padx=12)
        self.serial_combo = ctk.CTkComboBox(frame, values=["正在扫描..."],
                                            command=self._on_serial_selected)
        self.serial_combo.pack(fill="x", padx=12, pady=(2, 2))
        self._refresh_serial_ports()

        self.btn_serial = ctk.CTkButton(frame, text="连接串口",
                                        command=self._toggle_serial,
                                        height=34)
        self.btn_serial.pack(fill="x", padx=12, pady=(2, 6))

        ctk.CTkLabel(frame, text="——————————", text_color="gray").pack()

        # TCP
        ctk.CTkLabel(frame, text="网络 (TCP)",
                     font=ctk.CTkFont(size=13, weight="bold")).pack(anchor="w", padx=12)
        row1 = ctk.CTkFrame(frame)
        row1.pack(fill="x", padx=12, pady=2)
        ctk.CTkLabel(row1, text="IP 地址:", width=55).pack(side="left")
        self.tcp_ip = ctk.CTkEntry(row1, placeholder_text="10.109.232.90")
        self.tcp_ip.pack(side="left", fill="x", expand=True)

        row2 = ctk.CTkFrame(frame)
        row2.pack(fill="x", padx=12, pady=2)
        ctk.CTkLabel(row2, text="端口:", width=55).pack(side="left")
        self.tcp_port = ctk.CTkEntry(row2, width=80, placeholder_text="8080")
        self.tcp_port.pack(side="left")
        self.tcp_port.insert(0, "8080")

        self.btn_tcp = ctk.CTkButton(frame, text="连接网络",
                                     command=self._toggle_tcp, height=34)
        self.btn_tcp.pack(fill="x", padx=12, pady=(4, 6))

        ctk.CTkLabel(frame, text="——————————", text_color="gray").pack()

        # 状态
        ctk.CTkLabel(frame, text="连接状态",
                     font=ctk.CTkFont(size=13, weight="bold")).pack(anchor="w", padx=12, pady=(4, 4))
        self.status_serial = ctk.CTkLabel(frame, text="串口: 未连接", text_color="gray")
        self.status_serial.pack(anchor="w", padx=12)
        self.status_tcp = ctk.CTkLabel(frame, text="网络: 未连接", text_color="gray")
        self.status_tcp.pack(anchor="w", padx=12)

        ctk.CTkButton(frame, text="刷新串口列表", width=110,
                      command=self._refresh_serial_ports).pack(pady=(20, 4))

    def _build_center_panel(self):
        frame = ctk.CTkFrame(self)
        frame.grid(row=0, column=1, padx=4, pady=8, sticky="nsew")
        frame.grid_columnconfigure(0, weight=1)
        frame.grid_rowconfigure(0, weight=0)
        frame.grid_rowconfigure(1, weight=1)

        # 大字体实时数据
        data_frame = ctk.CTkFrame(frame, fg_color="transparent")
        data_frame.grid(row=0, column=0, sticky="ew", padx=12, pady=(10, 0))

        self.light_label = ctk.CTkLabel(data_frame, text="---- mV",
                                        font=ctk.CTkFont(size=48, weight="bold"))
        self.light_label.pack(side="left", padx=(0, 20))

        info_frame = ctk.CTkFrame(data_frame, fg_color="transparent")
        info_frame.pack(side="left")
        self.level_label = ctk.CTkLabel(info_frame, text="正常",
                                        font=ctk.CTkFont(size=24, weight="bold"),
                                        text_color="#44DD44")
        self.level_label.pack(anchor="w")
        self.buzzer_label = ctk.CTkLabel(info_frame, text="蜂鸣器: 关",
                                         font=ctk.CTkFont(size=16))
        self.buzzer_label.pack(anchor="w")
        self.th_label = ctk.CTkLabel(info_frame, text="阈值: —",
                                     font=ctk.CTkFont(size=14),
                                     text_color="gray")
        self.th_label.pack(anchor="w")

        # Matplotlib
        self.fig = Figure(figsize=(8, 3), dpi=100, facecolor="#2b2b2b")
        self.ax = self.fig.add_subplot(111)
        self.ax.set_facecolor("#1e1e1e")
        self.ax.tick_params(colors="white", labelsize=8)
        self.ax.spines["bottom"].set_color("#555")
        self.ax.spines["left"].set_color("#555")
        self.ax.spines["top"].set_visible(False)
        self.ax.spines["right"].set_visible(False)
        self.ax.set_ylabel("光照 (mV)", color="white", fontsize=9)
        self.ax.set_ylim(0, 3600)
        self.ax.yaxis.set_major_locator(plt().MultipleLocator(500))
        self.ax.grid(True, alpha=0.3, color="white")
        self.line, = self.ax.plot([], [], color="#00BFFF", linewidth=1.5)

        self.th_lines = {}
        for name, y, color in [
            ("强光阈值", 2000, "#FF4444"),
            ("偏亮阈值", 1500, "#FFAA00"),
            ("偏暗阈值", 800, "#44AAFF"),
            ("极暗阈值", 350, "#8844FF"),
        ]:
            line_hl = self.ax.axhline(y=y, color=color, linestyle="--",
                                      linewidth=0.8, alpha=0.6)
            self.th_lines[name] = line_hl
            self.ax.text(0, y, f" {name} ", color=color, fontsize=7,
                         va="center", ha="left", alpha=0.7)

        self.canvas = FigureCanvasTkAgg(self.fig, master=frame)
        self.canvas.get_tk_widget().grid(row=1, column=0, sticky="nsew",
                                         padx=4, pady=4)

    def _build_right_panel(self):
        frame = ctk.CTkFrame(self)
        frame.grid(row=0, column=2, padx=8, pady=8, sticky="nsew")
        frame.grid_columnconfigure(0, weight=1)

        ctk.CTkLabel(frame, text="设备控制",
                     font=ctk.CTkFont(size=16, weight="bold")).pack(pady=(12, 8))

        # 蜂鸣器
        ctk.CTkLabel(frame, text="蜂鸣器控制",
                     font=ctk.CTkFont(size=13, weight="bold")).pack(anchor="w", padx=12)
        row_btn = ctk.CTkFrame(frame)
        row_btn.pack(fill="x", padx=12, pady=2)
        ctk.CTkButton(row_btn, text="打开", width=55,
                      fg_color="#CC3333", hover_color="#FF4444",
                      command=lambda: self._send_command(
                          build_command_buzzer("on"))).pack(side="left", padx=2)
        ctk.CTkButton(row_btn, text="关闭", width=55,
                      fg_color="#555555",
                      command=lambda: self._send_command(
                          build_command_buzzer("off"))).pack(side="left", padx=2)
        ctk.CTkButton(row_btn, text="自动", width=55,
                      fg_color="#226622",
                      command=lambda: self._send_command(
                          build_command_buzzer("auto"))).pack(side="left", padx=2)

        # 阈值
        ctk.CTkLabel(frame, text="光照阈值 (mV)",
                     font=ctk.CTkFont(size=13, weight="bold")).pack(anchor="w", padx=12, pady=(20, 4))

        self.slider_th_high = self._make_slider(
            frame, "强光", 0, 3600, 2000)
        self.slider_th_mid_h = self._make_slider(
            frame, "偏亮", 0, 3600, 1500)
        self.slider_th_mid_l = self._make_slider(
            frame, "偏暗", 0, 3600, 800)
        self.slider_th_low = self._make_slider(
            frame, "极暗", 0, 3600, 350)

        ctk.CTkButton(frame, text="下发阈值到设备", height=36,
                      command=self._send_thresholds).pack(fill="x", padx=12, pady=(16, 4))

        ctk.CTkButton(frame, text="查询设备状态", height=32,
                      command=lambda: self._send_command(
                          build_command_get_status())).pack(fill="x", padx=12, pady=2)

    def _make_slider(self, parent, name, min_v, max_v, default):
        row = ctk.CTkFrame(parent, fg_color="transparent")
        row.pack(fill="x", padx=12, pady=3)

        lbl = ctk.CTkLabel(row, text=f"{name}:", width=50, anchor="w")
        lbl.pack(side="left")

        slider = ctk.CTkSlider(row, from_=min_v, to=max_v,
                               number_of_steps=72, width=90)
        slider.set(default)
        slider.pack(side="left", padx=4, fill="x", expand=True)

        val_label = ctk.CTkLabel(row, text=str(int(default)), width=45)
        val_label.pack(side="left")

        def on_slide(v):
            val_label.configure(text=str(int(v)))
            if not self._syncing_sliders:
                self._on_slider_change()

        slider.configure(command=on_slide)
        return slider

    def _build_log_panel(self):
        frame = ctk.CTkFrame(self)
        frame.grid(row=1, column=1, columnspan=2, padx=4, pady=(0, 8), sticky="nsew")
        frame.grid_columnconfigure(0, weight=1)
        frame.grid_rowconfigure(0, weight=0)
        frame.grid_rowconfigure(1, weight=1)

        header = ctk.CTkFrame(frame, fg_color="transparent")
        header.grid(row=0, column=0, sticky="ew", padx=8, pady=(4, 0))
        ctk.CTkLabel(header, text="通信日志",
                     font=ctk.CTkFont(size=13, weight="bold")).pack(side="left")
        ctk.CTkButton(header, text="清空", width=50,
                      command=self._clear_log).pack(side="right", padx=2)
        ctk.CTkButton(header, text="保存 TXT", width=70,
                      command=self._save_log_txt).pack(side="right", padx=2)
        ctk.CTkButton(header, text="保存 CSV", width=70,
                      command=self._save_log_csv).pack(side="right", padx=2)

        self.log_text = ctk.CTkTextbox(frame, font=ctk.CTkFont(size=11),
                                       wrap="word")
        self.log_text.grid(row=1, column=0, sticky="nsew", padx=8, pady=4)

    # ============================================================
    #  动作函数
    # ============================================================
    def _refresh_serial_ports(self):
        ports = SerialHandler.list_ports()
        if not ports:
            ports = ["未发现串口"]
        self.serial_combo.configure(values=ports)
        if ports and ports[0] != "未发现串口":
            self.serial_combo.set(ports[0])

    def _on_serial_selected(self, choice):
        pass

    def _toggle_serial(self):
        if self.serial.is_connected():
            self.serial.disconnect()
            self.btn_serial.configure(text="连接串口")
            self._log("[串口] 已断开")
        else:
            port = self.serial_combo.get()
            if port and port != "未发现串口":
                ok = self.serial.connect(port)
                if ok:
                    self.btn_serial.configure(text="断开串口")
                    self._sliders_synced = False  # 重新连接后允许同步一次
                    self._log(f"[串口] 已连接: {port}")
                else:
                    self._log(f"[串口] 连接失败: {port}")

    def _toggle_tcp(self):
        if self.tcp.is_connected():
            self.tcp.disconnect()
            self.btn_tcp.configure(text="连接网络")
            self._log("[网络] 已断开")
        else:
            host = self.tcp_ip.get().strip()
            port_str = self.tcp_port.get().strip()
            try:
                port = int(port_str)
            except ValueError:
                port = 8080
            ok = self.tcp.connect(host, port)
            if ok:
                self.btn_tcp.configure(text="断开网络")
                self._sliders_synced = False  # 重新连接后允许同步一次
                self._log(f"[网络] 已连接: {host}:{port}")
            else:
                self._log(f"[网络] 连接失败: {host}:{port}")

    def _on_status(self, stype, msg):
        if stype in ("Disconnected", "Error"):
            self.after(0, lambda: self.btn_serial.configure(text="连接串口"))
            self.after(0, lambda: self.btn_tcp.configure(text="连接网络"))
            self._log(f"[{stype}] {msg}")
        self.after(0, self._update_status_labels)

    def _on_data(self, data):
        self.data_queue.put(data)

    def _send_command(self, cmd_str):
        """发送指令：优先 TCP（双向可靠），串口仅做备选"""
        sent_tcp = False
        sent_serial = False
        # TCP 优先 — 双向通信可靠
        if self.tcp.is_connected():
            sent_tcp = self.tcp.send(cmd_str)
        # 串口备选（注意：本开发板串口下行 GPIO38 硬件不通，可能无效）
        if self.serial.is_connected():
            sent_serial = self.serial.send(cmd_str)

        if sent_tcp:
            self._log(f"[TCP] 发送 >>> {cmd_str}")
        elif sent_serial:
            self._log(f"[串口] 发送 >>> {cmd_str}  (⚠ 串口下行可能不通，建议使用网络连接)")
        else:
            self._log("发送 >>> 无可用连接! 请先连接网络(TCP)或串口")

    def _send_thresholds(self):
        th_high   = int(self.slider_th_high.get())
        th_mid_h  = int(self.slider_th_mid_h.get())
        th_mid_l  = int(self.slider_th_mid_l.get())
        th_low    = int(self.slider_th_low.get())

        # 校验：必须满足 th_high > th_mid_h > th_mid_l > th_low
        if not validate_thresholds(th_high, th_mid_h, th_mid_l, th_low):
            self._log("发送 >>> 阈值校验失败: 必须满足 强光 > 偏亮 > 偏暗 > 极暗")
            return

        cmd = build_command_set_threshold(th_high, th_mid_h, th_mid_l, th_low)
        self._send_command(cmd)

    def _on_slider_change(self):
        try:
            self.th_lines["强光阈值"].set_ydata(
                [self.slider_th_high.get(), self.slider_th_high.get()])
            self.th_lines["偏亮阈值"].set_ydata(
                [self.slider_th_mid_h.get(), self.slider_th_mid_h.get()])
            self.th_lines["偏暗阈值"].set_ydata(
                [self.slider_th_mid_l.get(), self.slider_th_mid_l.get()])
            self.th_lines["极暗阈值"].set_ydata(
                [self.slider_th_low.get(), self.slider_th_low.get()])
            self.canvas.draw_idle()
        except Exception:
            pass

    def _update_status_labels(self):
        s_text = f"串口: {'已连接' if self.serial.is_connected() else '未连接'}"
        t_text = f"网络: {'已连接' if self.tcp.is_connected() else '未连接'}"
        self.status_serial.configure(
            text=s_text,
            text_color="#44DD44" if self.serial.is_connected() else "gray")
        self.status_tcp.configure(
            text=t_text,
            text_color="#44DD44" if self.tcp.is_connected() else "gray")

    def _poll_data(self):
        try:
            while True:
                data = self.data_queue.get_nowait()
                self.latest_data = data
                self._update_display(data)
                self.history.append((datetime.now(), data["light"]))
                self._log_data(data)
        except queue.Empty:
            pass
        self.after(100, self._poll_data)

    def _sync_sliders_from_data(self, data):
        """仅在首次收到设备数据时同步滑块，之后由用户自由控制，避免每500ms覆盖用户拖拽值"""
        if self._sliders_synced:
            return
        keys = ("th_high", "th_mid_h", "th_mid_l", "th_low")
        sliders = (self.slider_th_high, self.slider_th_mid_h,
                   self.slider_th_mid_l, self.slider_th_low)
        self._syncing_sliders = True
        try:
            for key, slider in zip(keys, sliders):
                val = data.get(key)
                if val is not None:
                    slider.set(int(val))
        finally:
            self._syncing_sliders = False
        self._sliders_synced = True
        self._on_slider_change()

    def _update_display(self, data):
        light = data.get("light", 0)
        level = data.get("light_level", 2)
        level_name = data.get("light_level_name", "NORMAL")
        buzzer = data.get("buzzer", "off")
        th = (
            data.get("th_high", 2000),
            data.get("th_mid_h", 1500),
            data.get("th_mid_l", 800),
            data.get("th_low", 350),
        )

        color = LIGHT_COLORS.get(level, "#FFFFFF")
        self.light_label.configure(text=f"{light} mV", text_color=color)

        cn_name = LIGHT_NAMES_CN.get(level_name, level_name)
        self.level_label.configure(text=cn_name, text_color=color)

        bz_text = "蜂鸣器: 开" if buzzer == "on" else "蜂鸣器: 关"
        bz_color = "#FF4444" if buzzer == "on" else "#888888"
        self.buzzer_label.configure(text=bz_text, text_color=bz_color)

        self.th_label.configure(
            text=f"阈值: {th[0]}/{th[1]}/{th[2]}/{th[3]}")

        # 同步滑块到设备实际阈值
        self._sync_sliders_from_data(data)

    def _update_plot(self):
        if self.history:
            times = [t for t, v in self.history]
            values = [v for t, v in self.history]
            self.line.set_data(times, values)
            self.ax.set_xlim(times[0], times[-1])
            self.ax.xaxis.set_major_formatter(
                mdates.DateFormatter("%H:%M:%S"))
            self.fig.autofmt_xdate(rotation=30)
            self.canvas.draw_idle()
        self.after(PLOT_INTERVAL_MS, self._update_plot)

    # ============================================================
    #  日志
    # ============================================================
    def _log(self, msg):
        ts = datetime.now().strftime("%H:%M:%S")
        line = f"[{ts}] {msg}"
        self.log_lines.append(line)
        if len(self.log_lines) > LOG_MAX_LINES:
            self.log_lines = self.log_lines[-LOG_MAX_LINES:]
        self.after(0, self._refresh_log)

    def _log_data(self, data):
        light = data.get("light", 0)
        level_name = data.get("light_level_name", "?")
        cn_name = LIGHT_NAMES_CN.get(level_name, level_name)
        buzzer = "开" if data.get("buzzer") == "on" else "关"
        wifi = data.get("wifi", "?")
        msg = f"光照={light:4d} mV  等级={cn_name}  蜂鸣器={buzzer}  WiFi={wifi}"
        self._log(msg)

    def _refresh_log(self):
        text = "\n".join(self.log_lines[-200:])
        self.log_text.delete("1.0", "end")
        self.log_text.insert("1.0", text)
        self.log_text.see("end")

    def _clear_log(self):
        self.log_lines.clear()
        self.log_text.delete("1.0", "end")

    def _save_log_txt(self):
        path = f"log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"
        try:
            with open(path, "w", encoding="utf-8") as f:
                f.write("\n".join(self.log_lines))
            self._log(f"日志已保存: {path}")
        except Exception as e:
            self._log(f"保存 TXT 失败: {e}")

    def _save_log_csv(self):
        path = f"log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
        try:
            with open(path, "w", encoding="utf-8") as f:
                f.write("时间戳,光照(mV),等级,等级名称,蜂鸣器,蜂鸣器模式,"
                        "TH_HIGH,TH_MID_H,TH_MID_L,TH_LOW,WiFi,IP\n")
                if hasattr(self, '_csv_data'):
                    for d in self._csv_data:
                        f.write(
                            f"{d.get('ts','')},{d.get('light','')},{d.get('light_level','')},"
                            f"{d.get('light_level_name','')},{d.get('buzzer','')},"
                            f"{d.get('buzzer_mode','')},{d.get('th_high','')},{d.get('th_mid_h','')},"
                            f"{d.get('th_mid_l','')},{d.get('th_low','')},{d.get('wifi','')},{d.get('ip','')}\n"
                        )
            self._log(f"日志已保存: {path}")
        except Exception as e:
            self._log(f"保存 CSV 失败: {e}")

    def on_close(self):
        self.serial.disconnect()
        self.tcp.disconnect()
        self.destroy()


# ============================================================
def plt():
    import matplotlib.pyplot as _plt
    return _plt


# ============================================================
if __name__ == "__main__":
    app = LightMonitorApp()

    app._csv_data = []
    original_on_data = app._on_data

    def on_data_with_csv(data):
        app._csv_data.append(data)
        if len(app._csv_data) > 10000:
            app._csv_data = app._csv_data[-5000:]
        original_on_data(data)

    app.serial.on_data = on_data_with_csv
    app.tcp.on_data = on_data_with_csv

    app.protocol("WM_DELETE_WINDOW", app.on_close)
    app.mainloop()
