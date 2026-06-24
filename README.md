[Uploading README.md…]()
# -
实现对不同光照强度的跟踪和交互设备
# 智能光敏监控系统 | Intelligent Light Monitoring System

> **ESP32-S3** · TFT LCD · Photoresistor · Buzzer Alarm · WiFi TCP · Python上位机

[![Platform](https://img.shields.io/badge/platform-ESP32--S3-blue)](https://www.espressif.com/)
[![Framework](https://img.shields.io/badge/framework-ESP--IDF%20v5.1.2-red)](https://docs.espressif.com/projects/esp-idf/)
[![Python](https://img.shields.io/badge/python-3.11+-green)](https://www.python.org/)
[![License](https://img.shields.io/badge/license-MIT-yellow)](LICENSE)

---

## 📖 项目简介 | Overview

智能光敏监控系统是一套基于 **ESP32-S3** 的嵌入式 IoT 项目。通过光敏传感器实时采集环境光照强度，根据四级可配置阈值自动控制蜂鸣器告警，支持 LCD 液晶屏显示、三按键交互、串口 JSON 通信、WiFi TCP 网络通信，并配套 **Python PC 上位机软件** 实现数据可视化和远程控制。

The Intelligent Light Monitoring System is an ESP32-S3-based embedded IoT project. It collects ambient light intensity in real-time via a photoresistor sensor, automatically controls a buzzer alarm based on four configurable thresholds, supports TFT LCD display, three-button interaction, UART JSON communication, and WiFi TCP networking. A companion Python PC application provides data visualization and remote control.

---

## ✨ 核心功能 | Features

| 模块 | Module | 功能 |
|------|--------|------|
| 🎯 传感器采集 | Sensor | 光敏电阻 ADC 模拟量采集 + 8 阶滑动平均数字滤波 |
| 🧠 智能控制 | Control | 5 级光照判定 → 蜂鸣器分级响应 (常响/快闪/关闭/慢闪/中闪) |
| 🖥️ 人机交互 | HMI | 3 页面 TFT LCD 显示 + 3 按键 (短按/长按) |
| 📡 串口通信 | UART | JSON 格式上行数据 (500ms 周期) + 下行指令解析 |
| 🌐 网络通信 | Network | WiFi STA + TCP Server (端口 8080) |
| 💻 PC 上位机 | PC App | 实时数据大字显示 + 动态曲线图 + 阈值控制 + 日志保存 |

### 光照等级与蜂鸣器响应 | Light Levels & Buzzer

| 等级 | 名称 | 判定条件 (默认) | 蜂鸣器响应 |
|------|------|----------------|-----------|
| L0 | VERY_BRIGHT 极强光 | > 2000 mV | 🔔 持续鸣响 |
| L1 | BRIGHT 偏亮 | 1500 ~ 2000 mV | 🔔 持续鸣响 |
| L2 | NORMAL 正常 | 800 ~ 1500 mV | 🔕 关闭 |
| L3 | DIM 偏暗 | 350 ~ 800 mV | 🔔 慢闪 (0.33Hz) |
| L4 | VERY_DARK 极暗 | < 350 mV | 🔔 中速闪 (1Hz) |

> 💡 阈值可通过**按键、串口指令、TCP 指令、PC 上位机**四种方式实时修改。

---

## 🔧 硬件需求 | Hardware Requirements

### 物料清单 | BOM

| 器件 | 规格 | 数量 |
|------|------|------|
| ESP32-S3 开发板 | 普中 AI 机器视觉 | 1 |
| TFT LCD 显示屏 | ST7789 240×320 SPI | 1 |
| 光敏传感器模块 | 4 脚 (VCC/GND/AO/DO) | 1 |
| 有源蜂鸣器 | 3.3V，NPN 三极管驱动 | 1 |
| 按键 | 2 脚微动开关 | 3 |
| NPN 三极管 | S8050 | 1 |
| 电阻 | 1kΩ, 10kΩ | 各 1 |
| 面包板 + 杜邦线 | — | 若干 |

### 核心引脚连接 | Key Pinout

| GPIO | 外设 | 说明 |
|------|------|------|
| **GPIO5** | 光敏传感器 AO | ADC1_CH4，模拟输入 |
| **GPIO6** | 蜂鸣器 | 经 S8050 三极管驱动 |
| **GPIO2** | 板载 LED | 系统运行指示灯 |
| **GPIO15** | 按键 K1 | 页面切换 / 配置模式 |
| **GPIO17** | 按键 K2 | 阈值减小 |
| **GPIO8** | 按键 K3 | 阈值增大 |
| **GPIO21/47/48/42/41/14** | LCD | ST7789 SPI 接口 |
| **GPIO36** | UART0 TX | 接串口模块 RX |
| **GPIO38** | UART0 RX | 接串口模块 TX |

> 📋 完整引脚分配和接线步骤请参见 [`硬件连接说明书.md`](硬件连接说明书.md)

---

## 📂 项目结构 | Project Structure

```
qimozuoye/
├── main/                        # 嵌入式主程序 (C)
│   ├── main.c                   # FreeRTOS 任务管理、业务逻辑
│   ├── filter.h / filter.c      # 8 阶滑动平均数字滤波器
│   ├── protocol.h / protocol.c  # JSON 协议构造与解析
│   └── CMakeLists.txt
│
├── components/                  # 硬件驱动层
│   ├── LED/                     # LED 驱动 (GPIO2)
│   ├── BEEP/                    # 蜂鸣器驱动 (GPIO6)
│   ├── KEY/                     # 按键轮询驱动
│   ├── LCD/                     # ST7789 液晶驱动 (SPI)
│   ├── UART/                    # 串口驱动 (UART0)
│   ├── single-shotADC/          # ADC 单次采样
│   ├── GPTIM/                   # 通用定时器 (500ms)
│   ├── SPI/                     # SPI2 驱动
│   ├── wifi-sta/                # WiFi STA 连接
│   └── ...更多外设驱动
│
├── pc_software/                 # PC 上位机 (Python)
│   ├── main.py                  # GUI 主程序 (CustomTkinter + Matplotlib)
│   ├── protocol.py              # JSON 协议 (与嵌入式对齐)
│   ├── serial_handler.py        # 串口通信后台线程
│   ├── tcp_handler.py           # TCP Socket 通信线程
│   ├── verify_protocol.py       # 协议验证脚本
│   └── requirements.txt         # Python 依赖
│
├── 串口调试助手/                 # sscom33.exe 串口工具
├── 网络调试助手/                 # NetAssist.exe 网络工具
├── 完整使用说明书.md              # 完整文档
├── 硬件连接说明书.md              # 引脚分配与接线图
├── 使用说明书.md                  # 简明操作指南
├── CMakeLists.txt               # ESP-IDF 顶层构建文件
└── sdkconfig                    # ESP-IDF SDK 配置
```

### 嵌入式任务关系 | RTOS Task Graph

```
GPTIM (500ms ISR)
    ↓ flag_timer=1
sensor_task (等待 flag_timer)
    ↓ ADC → 滤波 → 等级判定 → 蜂鸣器控制
    ↓ 置位 EVT_SENSOR_READY
control_task (等待 EVT_SENSOR_READY)
    ↓ 读取 g_sys → 构造 JSON → 推送 UART/TCP 队列
    ↓
uart_tx_task (等队列) → uart_write_bytes
tcp_srv_task (select 轮询) → send()

key_scan_task (20ms) → GPIO 扫描 → 事件队列
    ↓
key_handler_task → 更新 g_sys (页面/模式/阈值)
    ↓
lcd_task (500ms) → 读取 g_sys → 刷新 LCD
```

---

## 🚀 快速开始 | Quick Start

### 1. 嵌入式端 | ESP32 Firmware

**环境要求**: ESP-IDF v5.1.2

```bash
# 进入工程目录
cd F:\qimozuoye

# 设置目标芯片
idf.py set-target esp32s3

# 编译
idf.py build

# 烧录 (将 COM4 替换为实际编程端口)
idf.py -p COM4 flash

# 查看串口输出
idf.py -p COM4 monitor
```

**修改 WiFi 配置** — 编辑 `components/wifi-sta/wifi-sta.h`:

```c
#define DEFAULT_SSID    "你的WiFi名称"
#define DEFAULT_PWD     "你的WiFi密码"
```

**修改默认阈值** — 编辑 `main/protocol.h`:

```c
#define DEFAULT_TH_HIGH         2000
#define DEFAULT_TH_MID_H        1500
#define DEFAULT_TH_MID_L        800
#define DEFAULT_TH_LOW          350
```

### 2. PC 上位机 | PC Application

**环境要求**: Python 3.11+

```bash
cd pc_software
pip install -r requirements.txt
python main.py
```

依赖: `customtkinter` `matplotlib` `pyserial` `pillow`

---

## 📡 通信协议 | Communication Protocol

### 串口参数 | UART Config

| 参数 | 值 |
|------|-----|
| 波特率 | 115200 bps |
| 数据位 | 8 |
| 停止位 | 1 |
| 校验 | 无 |

### 上行数据 (设备 → PC，每 500ms) | Uplink

```json
{
  "type": "data",
  "light": 985,
  "light_level": 2,
  "light_level_name": "NORMAL",
  "buzzer": "off",
  "buzzer_mode": "auto",
  "th_high": 2000,
  "th_mid_h": 1500,
  "th_mid_l": 800,
  "th_low": 350,
  "wifi": "connected",
  "ip": "10.109.232.90",
  "page": 0,
  "ts": 12500
}
```

### 下行指令 (PC → 设备) | Downlink Commands

```json
// 设置阈值 | Set thresholds
{"cmd":"set_threshold","th_high":2000,"th_mid_h":1500,"th_mid_l":800,"th_low":350}

// 蜂鸣器控制 | Buzzer control
{"cmd":"buzzer_ctrl","action":"on"}     // 强制开启
{"cmd":"buzzer_ctrl","action":"off"}    // 强制关闭
{"cmd":"buzzer_ctrl","action":"auto"}   // 恢复自动模式

// 查询状态 | Query status
{"cmd":"get_status"}
```

### TCP 网络 | TCP Network

| 参数 | 值 |
|------|-----|
| 协议 | TCP |
| 模式 | Server |
| 端口 | 8080 |
| 数据格式 | JSON (与串口协议相同) |
| 换行符 | `\r\n` |

---

## 🎮 按键操作 | Button Controls

| 按键 | 操作 | 功能 |
|------|------|------|
| **K1** | 短按 | LCD 页面切换 (P0→P1→P2→P0) |
| **K1** | 长按 2s | 进入/退出阈值配置模式 |
| **K2** | 短按 (配置模式) | 当前阈值 -50 |
| **K2** | 长按 (配置模式) | 当前阈值 -200 |
| **K3** | 短按 (配置模式) | 当前阈值 +50 |
| **K3** | 长按 (配置模式) | 当前阈值 +200 |

---

## 📺 LCD 页面 | Display Pages

### P0 — 实时数据 | Live Data
```
┌──────────────────┐
│  Light Monitor   │ 蓝底标题
├──────────────────┤
│ Light: 985 mV    │ 当前光敏值
│ NORMAL           │ 光照等级 (绿/黄/红)
│ Buzzer: OFF      │ 蜂鸣器状态
├──────────────────┤
│ Mode: auto       │ 蜂鸣器模式
│ TH:2000/1500/... │ 四档阈值
│ WiFi: OK         │ 网络状态
├──────────────────┤
│ P0 Data    K1>>  │ 底部导航
└──────────────────┘
```

### P1 — 阈值配置 | Thresholds
### P2 — 网络信息 | Network Info

> 📖 完整操作说明请参见 [`完整使用说明书.md`](完整使用说明书.md)

---

## 🖥️ PC 上位机 | PC Application

```
┌──────────────┬────────────────────────┬──────────────┐
│  通信连接     │    实时数据与曲线图      │   设备控制    │
│              │                        │              │
│ 串口: COM4   │  ┌─ 985 mV ─────────┐  │ 蜂鸣器:      │
│ [连接串口]   │  │ NORMAL           │  │ [开][关][自动]│
│              │  │ 蜂鸣器: 关        │  │              │
│ TCP:         │  └──────────────────┘  │ 阈值滑块:    │
│ IP: [      ] │                        │ 强光 ───●──  │
│ Port: 8080   │  ╱‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾╲   │ 偏亮 ───●──  │
│ [连接网络]   │ ╱  实时曲线 (120s)  ╲  │ 偏暗 ───●──  │
│              │╱  4条阈值虚线标注  ╲  │ 极暗 ───●──  │
├──────────────┴────────────────────────┴──────────────┤
│ 通信日志                          [清空][保存TXT][保存CSV] │
└─────────────────────────────────────────────────────┘
```

### 功能 | Features
- 📊 实时数据大字体显示，颜色随等级变化
- 📈 动态曲线图 (120 秒历史)，4 条阈值虚线标注
- 🎚️ 阈值滑块控制，一键下发到设备
- 🔊 蜂鸣器手动开/关/自动控制
- 📝 通信日志，支持 TXT/CSV 导出

---

## 🔍 故障排查 | Troubleshooting

| 现象 | 可能原因 | 解决方法 |
|------|---------|---------|
| 板载 LED 不亮 | 烧录失败 / 供电不足 | 重新烧录，检查 USB 线 |
| LCD 白屏 | SPI 接线错误 | 检查 GPIO21/47/42/41 |
| LCD 黑屏无背光 | BL 引脚未拉高 | 检查 GPIO14 |
| 光敏值始终为 0 | 传感器未接 | 检查 GPIO5 电压 |
| 蜂鸣器始终不响 | 阈值过高 / 驱动电路问题 | 听开机自检声判断 |
| 按键无反应 | 按键接触不良 | 测 GPIO 按下时是否为 0V |
| WiFi 连不上 | SSID/密码错误 | 修改 `wifi-sta.h` |
| 串口无数据 | TX 线接错 / 波特率不对 | 确认 GPIO36, 115200 |
| TCP 连不上 | IP 变化 / 端口被占用 | 查看 LCD P2 页当前 IP |

> 📖 更多故障排查请参见 [`完整使用说明书.md`](完整使用说明书.md)

---

## 🛠️ 技术栈 | Tech Stack

| 层级 | 技术 |
|------|------|
| 主控芯片 | ESP32-S3 |
| 开发框架 | ESP-IDF v5.1.2 |
| RTOS | FreeRTOS |
| 显示驱动 | ST7789 SPI (240×320) |
| 编程语言 | C (嵌入式) / Python (上位机) |
| GUI 框架 | CustomTkinter + Matplotlib |
| 通信协议 | UART 115200bps / TCP Server :8080 |
| 数据格式 | JSON |

---

## 📄 文档索引 | Documentation

- [`完整使用说明书.md`](完整使用说明书.md) — 完整使用手册
- [`硬件连接说明书.md`](硬件连接说明书.md) — 引脚分配与接线图
- [`使用说明书.md`](使用说明书.md) — 简明操作指南

---

## 📜 许可证 | License

MIT License — 详见 [LICENSE](LICENSE) 文件。

---

<p align="center">
  <b>Made with ❤️ on ESP32-S3</b><br>
  <sub>© 2026</sub>
</p>
