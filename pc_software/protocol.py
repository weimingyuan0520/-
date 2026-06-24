"""
protocol.py — JSON 协议处理 (与嵌入式端 protocol.h 对齐)
"""

import json
import time


def build_command_set_threshold(th_high, th_mid_h, th_mid_l, th_low):
    """构造下行指令: 设置阈值"""
    return json.dumps({
        "cmd": "set_threshold",
        "th_high": th_high,
        "th_mid_h": th_mid_h,
        "th_mid_l": th_mid_l,
        "th_low": th_low
    })


def build_command_buzzer(action):
    """构造下行指令: 控制蜂鸣器 (on/off/auto)"""
    return json.dumps({
        "cmd": "buzzer_ctrl",
        "action": action
    })


def build_command_get_status():
    """构造下行指令: 查询状态"""
    return json.dumps({"cmd": "get_status"})


def parse_uplink(json_str):
    """解析上行数据, 返回 dict 或 None"""
    try:
        data = json.loads(json_str.strip())
        if data.get("type") == "data":
            return data
    except (json.JSONDecodeError, ValueError):
        pass
    return None


LIGHT_LEVEL_NAMES = {
    0: "VERY_BRIGHT",
    1: "BRIGHT",
    2: "NORMAL",
    3: "DIM",
    4: "VERY_DARK",
}


def validate_thresholds(th_high, th_mid_h, th_mid_l, th_low):
    """校验阈值合法性：th_high > th_mid_h > th_mid_l > th_low"""
    return (th_high > th_mid_h and th_mid_h > th_mid_l and
            th_mid_l > th_low and th_high <= 3600 and th_low >= 0)
