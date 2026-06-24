/**
 * @file    protocol.h
 * @brief   统一 JSON 通信协议定义 — UART & TCP 共用
 */

#ifndef __PROTOCOL_H_
#define __PROTOCOL_H_

#include <stdint.h>
#include <stdbool.h>

/* ============================================================
 *  系统配置常量
 * ============================================================ */

#define TCP_SERVER_PORT         8080            /* TCP 监听端口 */
#define UPLINK_INTERVAL_MS      500             /* 上行数据间隔 (ms) */
#define SENSOR_SAMPLE_INTERVAL  500             /* 传感器采样间隔 (ms) */

/* ---- 默认阈值 (单位: mV, 0~3600) ---- */
/* 根据实测校准: 室内光 ~1000mV, 强光(手电) ~1700mV, 遮光 ~400mV */
#define DEFAULT_TH_HIGH         2000            /* 极强光 */
#define DEFAULT_TH_MID_H        1500            /* 偏亮 (手电照射可触发) */
#define DEFAULT_TH_MID_L        800             /* 偏暗 */
#define DEFAULT_TH_LOW          350             /* 极暗 */

/* ---- 光照等级 ---- */
typedef enum {
    LIGHT_LEVEL_VERY_BRIGHT = 0,  /* 强光 */
    LIGHT_LEVEL_BRIGHT      = 1,  /* 偏亮 */
    LIGHT_LEVEL_NORMAL      = 2,  /* 正常 */
    LIGHT_LEVEL_DIM         = 3,  /* 偏暗 */
    LIGHT_LEVEL_VERY_DARK   = 4,  /* 极暗 */
    LIGHT_LEVEL_MAX
} light_level_t;

/* ---- 蜂鸣器模式 ---- */
typedef enum {
    BUZZER_AUTO  = 0,   /* 自动模式 (由阈值控制) */
    BUZZER_ON    = 1,   /* 强制开启 */
    BUZZER_OFF   = 2,   /* 强制关闭 */
} buzzer_mode_t;

/* ---- LCD 显示页面 ---- */
typedef enum {
    LCD_PAGE_DATA       = 0,  /* 实时数据页 */
    LCD_PAGE_THRESHOLD  = 1,  /* 阈值配置页 */
    LCD_PAGE_NETWORK    = 2,  /* 网络信息页 */
    LCD_PAGE_MAX
} lcd_page_t;

/* ---- 系统工作模式 ---- */
typedef enum {
    MODE_NORMAL     = 0,  /* 正常监控模式 */
    MODE_TH_CONFIG  = 1,  /* 阈值配置模式 */
} system_mode_t;

/* ============================================================
 *  系统全局状态结构体 (多任务共享)
 * ============================================================ */
typedef struct {
    /* 传感器数据 */
    uint16_t    light_raw;          /* 光敏 ADC 原始值 (mV) */
    uint16_t    light_filtered;     /* 滤波后的光敏值 (mV) */
    light_level_t light_level;      /* 当前光照等级 */
    uint8_t     sensor_digital;     /* 数字光敏传感器值 (0/1) */

    /* 阈值配置 (可通过按键/串口/网络修改) */
    uint16_t    th_high;
    uint16_t    th_mid_h;
    uint16_t    th_mid_l;
    uint16_t    th_low;

    /* 蜂鸣器状态 */
    bool        buzzer_state;       /* 当前蜂鸣器电平 */
    buzzer_mode_t buzzer_mode;      /* 蜂鸣器工作模式 */

    /* 按键/LCD */
    lcd_page_t  lcd_page;           /* 当前 LCD 页面 */
    system_mode_t sys_mode;         /* 系统模式 */

    /* 网络状态 */
    bool        wifi_connected;     /* WiFi 是否已连接 */
    char        ip_addr[16];        /* IP 地址字符串 */
    int         tcp_client_fd;      /* 当前 TCP 客户端 socket (-1=无连接) */

    /* 串口通信 */
    bool        uart_enabled;

    /* 时间戳 (ms, 自启动起) */
    uint32_t    uptime_ms;

} sys_state_t;

/* ============================================================
 *  JSON 协议格式定义
 * ============================================================ */

/**
 * @brief 构造上行 JSON 数据报文
 * @param buf     输出缓冲区
 * @param bufsize 缓冲区大小
 * @param s       系统状态指针
 * @return        写入的字节数 (不含 '\0')
 */
int protocol_build_uplink(char *buf, int bufsize, sys_state_t *s);

/**
 * @brief 解析下行 JSON 指令
 * @param json  JSON 字符串
 * @param s     系统状态指针 (解析结果写回此处)
 * @param resp  响应输出缓冲区 (可 NULL)
 * @param respsize 响应缓冲区大小
 * @return      >=0 成功解析, <0 格式错误
 */
int protocol_parse_downlink(const char *json, sys_state_t *s,
                            char *resp, int respsize);

/**
 * @brief 获取光照等级名称字符串
 */
const char *light_level_name(light_level_t lv);

/**
 * @brief 获取蜂鸣器模式名称字符串
 */
const char *buzzer_mode_name(buzzer_mode_t mode);

#endif /* __PROTOCOL_H_ */
