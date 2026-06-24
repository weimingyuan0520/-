/**
 * @file    protocol.c
 * @brief   JSON 协议构造与解析实现
 * @note    嵌入式轻量实现，不依赖第三方 JSON 库，使用 sprintf/sscanf
 */

#include "protocol.h"
#include <stdio.h>
#include <string.h>

/* ============================================================
 *  辅助函数
 * ============================================================ */

const char *light_level_name(light_level_t lv)
{
    switch (lv) {
        case LIGHT_LEVEL_VERY_BRIGHT: return "VERY_BRIGHT";
        case LIGHT_LEVEL_BRIGHT:      return "BRIGHT";
        case LIGHT_LEVEL_NORMAL:      return "NORMAL";
        case LIGHT_LEVEL_DIM:         return "DIM";
        case LIGHT_LEVEL_VERY_DARK:   return "VERY_DARK";
        default:                      return "UNKNOWN";
    }
}

const char *buzzer_mode_name(buzzer_mode_t mode)
{
    switch (mode) {
        case BUZZER_AUTO: return "auto";
        case BUZZER_ON:   return "on";
        case BUZZER_OFF:  return "off";
        default:          return "unknown";
    }
}

/* ============================================================
 *  上行 JSON 构造
 * ============================================================ */

int protocol_build_uplink(char *buf, int bufsize, sys_state_t *s)
{
    return snprintf(buf, bufsize,
        "{"
        "\"type\":\"data\","
        "\"light\":%u,"
        "\"light_level\":%d,"
        "\"light_level_name\":\"%s\","
        "\"buzzer\":\"%s\","
        "\"buzzer_mode\":\"%s\","
        "\"th_high\":%u,"
        "\"th_mid_h\":%u,"
        "\"th_mid_l\":%u,"
        "\"th_low\":%u,"
        "\"wifi\":\"%s\","
        "\"ip\":\"%s\","
        "\"page\":%d,"
        "\"ts\":%lu"
        "}",
        s->light_filtered,
        (int)s->light_level,
        light_level_name(s->light_level),
        s->buzzer_state ? "on" : "off",
        buzzer_mode_name(s->buzzer_mode),
        s->th_high,
        s->th_mid_h,
        s->th_mid_l,
        s->th_low,
        s->wifi_connected ? "connected" : "disconnected",
        s->ip_addr,
        (int)s->lcd_page,
        (unsigned long)s->uptime_ms
    );
}

/* ============================================================
 *  下行 JSON 解析 (简易字符串匹配)
 * ============================================================ */

int protocol_parse_downlink(const char *json, sys_state_t *s,
                            char *resp, int respsize)
{
    if (!json || !s) return -1;

    /* ---- 1. 设置阈值指令 ---- */
    if (strstr(json, "\"set_threshold\"")) {
        int th, tmh, tml, tl;
        int n = sscanf(json,
            "{\"cmd\":\"set_threshold\",\"th_high\":%d,\"th_mid_h\":%d,\"th_mid_l\":%d,\"th_low\":%d}",
            &th, &tmh, &tml, &tl);
        if (n == 4) {
            /* 阈值合法性检查 */
            if (th > tmh && tmh > tml && tml > tl &&
                th <= 3600 && tl >= 0) {
                s->th_high  = (uint16_t)th;
                s->th_mid_h = (uint16_t)tmh;
                s->th_mid_l = (uint16_t)tml;
                s->th_low   = (uint16_t)tl;
                if (resp) {
                    snprintf(resp, respsize,
                        "{\"type\":\"ack\",\"cmd\":\"set_threshold\",\"result\":\"ok\"}");
                }
                return 0;
            }
        }
        if (resp) {
            snprintf(resp, respsize,
                "{\"type\":\"ack\",\"cmd\":\"set_threshold\",\"result\":\"fail\",\"msg\":\"invalid values\"}");
        }
        return -2;
    }

    /* ---- 2. 蜂鸣器控制指令 ---- */
    if (strstr(json, "\"buzzer_ctrl\"")) {
        if (strstr(json, "\"auto\"")) {
            s->buzzer_mode = BUZZER_AUTO;
            if (resp) snprintf(resp, respsize,
                "{\"type\":\"ack\",\"cmd\":\"buzzer_ctrl\",\"action\":\"auto\"}");
            return 0;
        }
        if (strstr(json, "\"on\"")) {
            s->buzzer_mode = BUZZER_ON;
            if (resp) snprintf(resp, respsize,
                "{\"type\":\"ack\",\"cmd\":\"buzzer_ctrl\",\"action\":\"on\"}");
            return 0;
        }
        if (strstr(json, "\"off\"")) {
            s->buzzer_mode = BUZZER_OFF;
            if (resp) snprintf(resp, respsize,
                "{\"type\":\"ack\",\"cmd\":\"buzzer_ctrl\",\"action\":\"off\"}");
            return 0;
        }
        return -3;
    }

    /* ---- 3. 查询状态指令 ---- */
    if (strstr(json, "\"get_status\"")) {
        if (resp) {
            protocol_build_uplink(resp, respsize, s);
        }
        return 0;
    }

    /* 未知指令 */
    if (resp) {
        snprintf(resp, respsize,
            "{\"type\":\"ack\",\"result\":\"fail\",\"msg\":\"unknown command\"}");
    }
    return -1;
}
