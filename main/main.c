/**
 * @file    main.c
 * @brief   智能光敏监控系统 — 嵌入式端主程序 (ESP32-S3 / FreeRTOS)
 * @note    五大核心模块：传感器采集、智能控制、人机交互、串口通信、网络通信
 * @fix     v1.1 — 修复LCD对齐、蜂鸣器误触发、按键初始化
 */

/* ============================================================
 *  Includes
 * ============================================================ */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

/* FreeRTOS */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

/* ESP-IDF */
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/uart.h"

/* lwIP / socket */
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/err.h"

/* 硬件驱动 */
#include "beep.h"
#include "key.h"
#include "lcd.h"
#include "myuart.h"
#include "single-shotADC.h"
#include "gptim.h"
#include "LED.h"
#include "wifi-sta.h"

/* 应用层 */
#include "filter.h"
#include "protocol.h"

/* ============================================================
 *  日志标签
 * ============================================================ */
static const char *TAG_MAIN = "main";

/* ============================================================
 *  全局状态 & 同步对象
 * ============================================================ */
static sys_state_t      g_sys;              /* 系统全局状态 */
static SemaphoreHandle_t g_state_mutex;     /* 保护 g_sys 的互斥锁 */
static SemaphoreHandle_t g_lcd_mutex;       /* LCD 访问互斥锁 */
static QueueHandle_t     g_uart_tx_queue;   /* 串口发送队列 (char*) */
static QueueHandle_t     g_tcp_tx_queue;    /* TCP 发送队列 (char*) */
static QueueHandle_t     g_cmd_queue;       /* 接收到的指令队列 (char*) */
static EventGroupHandle_t g_evt;            /* 系统事件标志组 */

/* 事件位 */
#define EVT_SENSOR_READY    BIT0    /* 传感器数据已更新 */
#define EVT_KEY_PRESSED     BIT1    /* 有按键事件 */
#define EVT_CMD_RECEIVED    BIT2    /* 收到外部指令 */
#define EVT_WIFI_READY      BIT3    /* WiFi 已连接并获取 IP */

/* ============================================================
 *  非阻塞按键扫描 (支持短按/长按)
 * ============================================================ */
#define KEY_LONG_PRESS_MS   2000    /* 长按阈值 2 秒 */
#define KEY_SCAN_PERIOD_MS  20      /* 扫描周期 20ms */

typedef struct {
    uint8_t  gpio_num;
    uint8_t  key_id;             /* KEY1_PRESSED ~ KEY4_PRESSED */
    bool     prev_state;         /* 上一次电平 (true=释放, false=按下) */
    uint32_t press_start_ms;     /* 按下时刻 */
    bool     long_triggered;     /* 长按是否已触发 */
    bool     pressed;            /* 当前是否处于按下状态 */
} key_ctx_t;

static key_ctx_t g_keys[4];
static uint32_t  g_key_uptime_ms = 0;  /* 按键任务维护的本地时间戳 */

/* 按键事件类型 */
typedef enum {
    KEY_EVT_NONE = 0,
    KEY_EVT_SHORT,
    KEY_EVT_LONG,
} key_evt_type_t;

typedef struct {
    uint8_t       key_id;
    key_evt_type_t evt;
} key_event_t;

static QueueHandle_t g_key_queue;  /* 按键事件队列 */

/* ---- 初始化按键上下文 ---- */
static void key_ctx_init(void)
{
    for (int i = 0; i < 4; i++) {
        g_keys[i].prev_state = true;   /* 初始化为"释放"状态 (上拉=高电平) */
        g_keys[i].pressed = false;
        g_keys[i].press_start_ms = 0;
        g_keys[i].long_triggered = false;
    }
    g_keys[0].gpio_num = KEY1_GPIO_NUM;  g_keys[0].key_id = KEY1_PRESSED;
    g_keys[1].gpio_num = KEY2_GPIO_NUM;  g_keys[1].key_id = KEY2_PRESSED;
    g_keys[2].gpio_num = KEY3_GPIO_NUM;  g_keys[2].key_id = KEY3_PRESSED;
    g_keys[3].gpio_num = KEY4_GPIO_NUM;  g_keys[3].key_id = KEY4_PRESSED;
}

/* ---- 非阻塞扫描所有按键 (每 KEY_SCAN_PERIOD_MS 调用一次) ---- */
static void key_scan_all(void)
{
    for (int i = 0; i < 4; i++) {
        int level = gpio_get_level(g_keys[i].gpio_num);  /* 0=按下, 1=释放 */
        bool is_pressed = (level == 0);

        if (!g_keys[i].prev_state && is_pressed) {
            /* 下降沿 -> 开始计时 */
            g_keys[i].pressed = true;
            g_keys[i].press_start_ms = g_key_uptime_ms;
            g_keys[i].long_triggered = false;
        }
        else if (g_keys[i].pressed && is_pressed) {
            /* 持续按下 -> 检查长按 */
            if (!g_keys[i].long_triggered &&
                (g_key_uptime_ms - g_keys[i].press_start_ms) >= KEY_LONG_PRESS_MS) {
                g_keys[i].long_triggered = true;
                key_event_t evt = { .key_id = g_keys[i].key_id, .evt = KEY_EVT_LONG };
                xQueueSend(g_key_queue, &evt, 0);
            }
        }
        else if (g_keys[i].pressed && !is_pressed) {
            /* 上升沿 -> 释放 */
            g_keys[i].pressed = false;
            if (!g_keys[i].long_triggered) {
                /* 短按: 在长按触发之前释放 */
                key_event_t evt = { .key_id = g_keys[i].key_id, .evt = KEY_EVT_SHORT };
                xQueueSend(g_key_queue, &evt, 0);
            }
            g_keys[i].press_start_ms = 0;
        }

        g_keys[i].prev_state = is_pressed;
    }
    g_key_uptime_ms += KEY_SCAN_PERIOD_MS;
}

/* ============================================================
 *  蜂鸣器模式控制 (基于 500ms 定时器节拍)
 * ============================================================ */
static uint8_t g_buzzer_tick = 0;

static void buzzer_pattern_update(sys_state_t *s)
{
    g_buzzer_tick++;

    if (s->buzzer_mode == BUZZER_ON) {
        beep_on();
        s->buzzer_state = true;
        ESP_LOGI(TAG_MAIN, "BZ: MODE=ON -> beep ON");
        return;
    }
    if (s->buzzer_mode == BUZZER_OFF) {
        beep_off();
        s->buzzer_state = false;
        ESP_LOGI(TAG_MAIN, "BZ: MODE=OFF -> beep OFF");
        return;
    }

    /* AUTO 模式 */
    switch (s->light_level) {
        case LIGHT_LEVEL_VERY_BRIGHT:
            beep_on();
            s->buzzer_state = true;
            ESP_LOGI(TAG_MAIN, "BZ: AUTO, level=VERY_BRIGHT(%d), light=%u -> ON", (int)s->light_level, s->light_filtered);
            break;
        case LIGHT_LEVEL_BRIGHT:
            if (g_buzzer_tick % 1 == 0) {
                if (s->buzzer_state) { beep_off(); s->buzzer_state = false; }
                else                 { beep_on();  s->buzzer_state = true;  }
            }
            break;
        case LIGHT_LEVEL_NORMAL:
            beep_off();
            s->buzzer_state = false;
            break;
        case LIGHT_LEVEL_DIM:
            if (g_buzzer_tick % 3 == 0) {
                if (s->buzzer_state) { beep_off(); s->buzzer_state = false; }
                else                 { beep_on();  s->buzzer_state = true;  }
            }
            break;
        case LIGHT_LEVEL_VERY_DARK:
            if (g_buzzer_tick % 1 == 0) {
                if (s->buzzer_state) { beep_off(); s->buzzer_state = false; }
                else                 { beep_on();  s->buzzer_state = true;  }
            }
            break;
        default:
            break;
    }
}

/* ============================================================
 *  光照等级判定
 * ============================================================ */
static light_level_t calc_light_level(uint16_t light_mv, sys_state_t *s)
{
    if (light_mv > s->th_high)       return LIGHT_LEVEL_VERY_BRIGHT;
    if (light_mv > s->th_mid_h)      return LIGHT_LEVEL_BRIGHT;
    if (light_mv >= s->th_mid_l)     return LIGHT_LEVEL_NORMAL;
    if (light_mv >= s->th_low)       return LIGHT_LEVEL_DIM;
    return LIGHT_LEVEL_VERY_DARK;
}

/* ============================================================
 *  LCD — 240x320 / 16x32px 字符 / 15列×9行
 * ============================================================ */
#define LCD_Y(line)     ((line-1)*32 + 8)
#define LCD_COLS        15

static void lcd_safe_str(uint8_t line, uint8_t col, const char *s,
                         uint16_t fg, uint16_t bg)
{
    char buf[16];
    strncpy(buf, s, LCD_COLS);
    buf[LCD_COLS] = '\0';
    lcd_show_string(line, col, buf, fg, bg);
}

static void lcd_page_show_data(sys_state_t *s)
{
    char buf[16];
    int y;

    /* line 1: 标题栏 */
    y = LCD_Y(1);
    lcd_fill(0, y, 239, y + 31, BLUE);
    lcd_safe_str(1, 1, "Light Monitor", WHITE, BLUE);

    /* line 2-4: 数据区 */
    y = LCD_Y(2);
    lcd_fill(0, y, 239, y + 95, BLACK);

    snprintf(buf, sizeof(buf), "Light:%4u mV", s->light_filtered);
    lcd_safe_str(2, 1, buf, YELLOW, BLACK);

    uint16_t lv_color = GREEN;
    if (s->light_level == LIGHT_LEVEL_VERY_BRIGHT ||
        s->light_level == LIGHT_LEVEL_VERY_DARK)   lv_color = RED;
    else if (s->light_level == LIGHT_LEVEL_BRIGHT ||
             s->light_level == LIGHT_LEVEL_DIM)    lv_color = YELLOW;
    snprintf(buf, sizeof(buf), "%-15s", light_level_name(s->light_level));
    lcd_safe_str(3, 1, buf, lv_color, BLACK);

    snprintf(buf, sizeof(buf), "Buzzer: %-7s", s->buzzer_state ? "ON" : "OFF");
    lcd_safe_str(4, 1, buf, s->buzzer_state ? RED : GREEN, BLACK);

    /* line 5: 分隔 */
    y = LCD_Y(5);
    lcd_fill(0, y, 239, y + 31, GRAY);
    lcd_safe_str(5, 1, "--- Status ---", WHITE, GRAY);

    /* line 6-8: 状态 */
    y = LCD_Y(6);
    lcd_fill(0, y, 239, y + 95, BLACK);

    snprintf(buf, sizeof(buf), "Mode: %-9s", buzzer_mode_name(s->buzzer_mode));
    lcd_safe_str(6, 1, buf, CYAN, BLACK);

    snprintf(buf, sizeof(buf), "TH:%u/%u/%u/%u",
             s->th_high, s->th_mid_h, s->th_mid_l, s->th_low);
    lcd_safe_str(7, 1, buf, LGRAY, BLACK);

    snprintf(buf, sizeof(buf), "WiFi: %-9s", s->wifi_connected ? "OK" : "---");
    lcd_safe_str(8, 1, buf, s->wifi_connected ? GREEN : LGRAY, BLACK);

    /* line 9: 底部 */
    y = LCD_Y(9);
    lcd_fill(0, y, 239, 319, DARKBLUE);
    lcd_safe_str(9, 1, "P0 Data   K1>>", WHITE, DARKBLUE);
}

static void lcd_page_show_threshold(sys_state_t *s)
{
    char buf[16];
    int y;

    y = LCD_Y(1);
    lcd_fill(0, y, 239, y + 31, BLUE);
    lcd_safe_str(1, 1, "Thresholds", WHITE, BLUE);

    y = LCD_Y(2);
    lcd_fill(0, y, 239, y + 127, BLACK);

    snprintf(buf, sizeof(buf), "TH_HIGH :%4u mV", s->th_high);
    lcd_safe_str(2, 1, buf, RED, BLACK);

    snprintf(buf, sizeof(buf), "TH_MID_H:%4u mV", s->th_mid_h);
    lcd_safe_str(3, 1, buf, YELLOW, BLACK);

    snprintf(buf, sizeof(buf), "TH_MID_L:%4u mV", s->th_mid_l);
    lcd_safe_str(4, 1, buf, CYAN, BLACK);

    snprintf(buf, sizeof(buf), "TH_LOW  :%4u mV", s->th_low);
    lcd_safe_str(5, 1, buf, GREEN, BLACK);

    y = LCD_Y(6);
    lcd_fill(0, y, 239, y + 95, BLACK);
    if (s->sys_mode == MODE_TH_CONFIG) {
        lcd_safe_str(6, 1, "** CONFIG MODE", RED, BLACK);
        lcd_safe_str(7, 1, "K2:-      K3:+", YELLOW, BLACK);
        lcd_safe_str(8, 1, "K1:Exit Config", WHITE, BLACK);
    } else {
        lcd_safe_str(7, 1, "Hold K1 2s to", LGRAY, BLACK);
        lcd_safe_str(8, 1, "enter config", LGRAY, BLACK);
    }

    y = LCD_Y(9);
    lcd_fill(0, y, 239, 319, DARKBLUE);
    lcd_safe_str(9, 1, "P1 Thresh  K1>>", WHITE, DARKBLUE);
}

static void lcd_page_show_network(sys_state_t *s)
{
    char buf[16];
    int y;

    /* line 1: 标题 */
    y = LCD_Y(1);
    lcd_fill(0, y, 239, y + 31, BLUE);
    lcd_safe_str(1, 1, "Network Info", WHITE, BLUE);

    /* line 2-8: 信息 */
    y = LCD_Y(2);
    lcd_fill(0, y, 239, y + 223, BLACK);

    snprintf(buf, sizeof(buf), "WiFi: %-9s", s->wifi_connected ? "OK" : "FAIL");
    lcd_safe_str(2, 1, buf, s->wifi_connected ? GREEN : RED, BLACK);

    snprintf(buf, sizeof(buf), "SSID: %-9s", DEFAULT_SSID);
    lcd_safe_str(3, 1, buf, CYAN, BLACK);

    lcd_safe_str(4, 1, "IP:", WHITE, BLACK);
    lcd_safe_str(5, 1, s->ip_addr, YELLOW, BLACK);

    snprintf(buf, sizeof(buf), "Port: %-5d TCP", TCP_SERVER_PORT);
    lcd_safe_str(6, 1, buf, GREEN, BLACK);

    lcd_safe_str(7, 1, "UART0  115200", LGRAY, BLACK);

    snprintf(buf, sizeof(buf), "Client: %-7s",
             s->tcp_client_fd >= 0 ? "OK" : "---");
    lcd_safe_str(8, 1, buf, s->tcp_client_fd >= 0 ? GREEN : LGRAY, BLACK);

    /* line 9: 底部 */
    y = LCD_Y(9);
    lcd_fill(0, y, 239, 319, DARKBLUE);
    lcd_safe_str(9, 1, "P2 Network K1>>", WHITE, DARKBLUE);
}

static void lcd_update(sys_state_t *s)
{
    /* LCD 互斥访问，防止 WiFi 事件回调冲突 */
    if (xSemaphoreTake(g_lcd_mutex, pdMS_TO_TICKS(300)) != pdTRUE) return;

    switch (s->lcd_page) {
        case LCD_PAGE_DATA:
            lcd_page_show_data(s);
            break;
        case LCD_PAGE_THRESHOLD:
            lcd_page_show_threshold(s);
            break;
        case LCD_PAGE_NETWORK:
            lcd_page_show_network(s);
            break;
        default:
            break;
    }

    xSemaphoreGive(g_lcd_mutex);
}

/* ============================================================
 *  发送辅助函数 (将 JSON 字符串入队)
 * ============================================================ */
#define TX_MSG_MAX_LEN  512

static char *safe_strdup(const char *src)
{
    if (!src) return NULL;
    size_t len = strlen(src) + 1;
    char *dst = (char *)malloc(len);
    if (dst) memcpy(dst, src, len);
    return dst;
}

static void send_to_uart(const char *msg)
{
    if (!msg) return;
    char *dup = safe_strdup(msg);
    if (dup && g_uart_tx_queue) {
        xQueueSend(g_uart_tx_queue, &dup, portMAX_DELAY);
    }
}

static void send_to_tcp(const char *msg)
{
    if (!msg) return;
    char *dup = safe_strdup(msg);
    if (dup && g_tcp_tx_queue) {
        xQueueSend(g_tcp_tx_queue, &dup, portMAX_DELAY);
    }
}

/* ============================================================
 *  Task 1: 传感器采集任务 (500ms 周期)
 * ============================================================ */
static void task_sensor(void *pvParameters)
{
    sliding_filter_t filter;
    filter_init(&filter);
    bool filter_ready = false;

    ESP_LOGI(TAG_MAIN, "Sensor task started");

    /* 等待滤波器填满再启用蜂鸣器控制 */
    for (;;) {
        /* 等待定时器节拍 (500ms) */
        while (flag_timer == 0) { vTaskDelay(pdMS_TO_TICKS(10)); }
        flag_timer = 0;

        /* 读取光敏传感器 ADC (反转: 光强↑→LDR电阻↓→电压↓→反转后值↑) */
        int raw_mv = 3600 - adc_read_light_sensor();
        if (raw_mv < 0) raw_mv = 0;
        if (raw_mv > 3600) raw_mv = 3600;

        /* 数字滤波 */
        uint16_t filtered = filter_update(&filter, (uint16_t)raw_mv);

        /* 前8次用于填充滤波器，不触发控制 */
        if (!filter_ready) {
            static int warmup = 0;
            warmup++;
            if (warmup >= FILTER_WINDOW_SIZE) {
                filter_ready = true;
                ESP_LOGI(TAG_MAIN, "Filter ready, first value=%u mV", filtered);
            }
            /* 预热期间写入状态但标记为正常光照 (避免误触发) */
            if (xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                g_sys.light_raw      = (uint16_t)raw_mv;
                g_sys.light_filtered = filtered;
                g_sys.sensor_digital = 0;
                if (!filter_ready) {
                    g_sys.light_level = LIGHT_LEVEL_NORMAL;  /* 预热期强制正常 */
                }
                g_sys.uptime_ms     += SENSOR_SAMPLE_INTERVAL;
                xSemaphoreGive(g_state_mutex);
            }
            if (!filter_ready) continue;
        }

        /* 更新全局状态 */
        if (xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            g_sys.light_raw      = (uint16_t)raw_mv;
            g_sys.light_filtered = filtered;
            g_sys.sensor_digital = 0;
            g_sys.light_level    = calc_light_level(filtered, &g_sys);
            g_sys.uptime_ms     += SENSOR_SAMPLE_INTERVAL;

            /* === 蜂鸣器直接控制 (传感器任务内, 无需等控制任务) === */
            if (g_sys.buzzer_mode == BUZZER_AUTO) {
                if (filtered > g_sys.th_high || filtered > g_sys.th_mid_h) {
                    beep_on();
                    g_sys.buzzer_state = true;
                } else if (filtered >= g_sys.th_mid_l) {
                    beep_off();
                    g_sys.buzzer_state = false;
                }
            }

            xSemaphoreGive(g_state_mutex);
        }

        /* 通知控制任务 */
        xEventGroupSetBits(g_evt, EVT_SENSOR_READY);
    }
}

/* ============================================================
 *  Task 2: 智能控制决策与执行任务
 * ============================================================ */
static void task_control(void *pvParameters)
{
    ESP_LOGI(TAG_MAIN, "Control task started");
    char tx_buf[TX_MSG_MAX_LEN];
    light_level_t last_level = LIGHT_LEVEL_MAX;

    for (;;) {
        EventBits_t bits = xEventGroupWaitBits(g_evt, EVT_SENSOR_READY,
                                                pdTRUE,
                                                pdFALSE,
                                                pdMS_TO_TICKS(1000));
        if ((bits & EVT_SENSOR_READY) == 0) continue;

        /* === 持锁期间直接操作 g_sys，消除竞态窗口 === */
        if (xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(100)) != pdTRUE) continue;

        /* 等级变化日志 */
        if (g_sys.light_level != last_level) {
            ESP_LOGI(TAG_MAIN, "Level: %s(%d) -> %s(%d)  light=%u mV",
                     light_level_name(last_level), (int)last_level,
                     light_level_name(g_sys.light_level), (int)g_sys.light_level,
                     g_sys.light_filtered);
            last_level = g_sys.light_level;
        }

        /* === 蜂鸣器控制 (直接阈值比较，无中间层) === */
        bool buzzer_before = g_sys.buzzer_state;
        g_buzzer_tick++;

        /* 每 2 秒输出一次诊断 (4 个节拍) */
        if (g_buzzer_tick % 4 == 0) {
            ESP_LOGI(TAG_MAIN, "DIAG: lv=%u th=%u/%u/%u/%u mode=%s level=%d",
                     g_sys.light_filtered,
                     g_sys.th_high, g_sys.th_mid_h,
                     g_sys.th_mid_l, g_sys.th_low,
                     buzzer_mode_name(g_sys.buzzer_mode),
                     (int)g_sys.light_level);
        }

        if (g_sys.buzzer_mode == BUZZER_ON) {
            beep_on();
            g_sys.buzzer_state = true;
        } else if (g_sys.buzzer_mode == BUZZER_OFF) {
            beep_off();
            g_sys.buzzer_state = false;
        } else { /* AUTO: 直接用 light_filtered 比较阈值 */
            uint16_t lv = g_sys.light_filtered;
            if (lv > g_sys.th_high) {
                beep_on();
                g_sys.buzzer_state = true;
            } else if (lv > g_sys.th_mid_h) {
                /* BRIGHT: 持续响 (与 VERY_BRIGHT 相同，只在 LCD 上区分等级) */
                beep_on();
                g_sys.buzzer_state = true;
            } else if (lv >= g_sys.th_mid_l) {
                /* NORMAL: 关闭 */
                beep_off();
                g_sys.buzzer_state = false;
            } else if (lv >= g_sys.th_low) {
                /* DIM: 慢速 */
                if (g_buzzer_tick % 3 == 0) {
                    if (g_sys.buzzer_state) { beep_off(); g_sys.buzzer_state = false; }
                    else { beep_on(); g_sys.buzzer_state = true; }
                }
            } else {
                /* VERY_DARK: 中速 */
                if (g_buzzer_tick % 1 == 0) {
                    if (g_sys.buzzer_state) { beep_off(); g_sys.buzzer_state = false; }
                    else { beep_on(); g_sys.buzzer_state = true; }
                }
            }
        }

        if (g_sys.buzzer_state != buzzer_before) {
            ESP_LOGI(TAG_MAIN, "BZ: %s->%s lv=%u mV > TH=%u,%u,%u,%u",
                     buzzer_before ? "ON" : "OFF",
                     g_sys.buzzer_state ? "ON" : "OFF",
                     g_sys.light_filtered,
                     g_sys.th_high, g_sys.th_mid_h, g_sys.th_mid_l, g_sys.th_low);
        }

        /* 构造 JSON (仍在持锁中，确保数据一致) */
        int len = protocol_build_uplink(tx_buf, sizeof(tx_buf), &g_sys);

        xSemaphoreGive(g_state_mutex);
        /* ========== 释放锁 ========== */

        if (len > 0) {
            send_to_uart(tx_buf);
            send_to_tcp(tx_buf);
        }
    }
}

/* ============================================================
 *  Task 3: 按键扫描任务 (20ms 周期)
 * ============================================================ */
static void task_key(void *pvParameters)
{
    ESP_LOGI(TAG_MAIN, "Key task started");
    for (;;) {
        key_scan_all();
        vTaskDelay(pdMS_TO_TICKS(KEY_SCAN_PERIOD_MS));
    }
}

/* ============================================================
 *  Task 3b: 按键事件处理 (从队列取事件)
 * ============================================================ */
static void task_key_handler(void *pvParameters)
{
    ESP_LOGI(TAG_MAIN, "Key handler task started");
    key_event_t evt;

    for (;;) {
        if (xQueueReceive(g_key_queue, &evt, portMAX_DELAY) != pdTRUE) continue;
        if (xSemaphoreTake(g_state_mutex, portMAX_DELAY) != pdTRUE) continue;

        /* K1: 页面切换 / 阈值配置 */
        if (evt.key_id == KEY1_PRESSED) {
            if (evt.evt == KEY_EVT_SHORT) {
                if (g_sys.sys_mode == MODE_TH_CONFIG) {
                    g_sys.sys_mode = MODE_NORMAL;
                    ESP_LOGI(TAG_MAIN, "Exit threshold config");
                } else {
                    g_sys.lcd_page = (g_sys.lcd_page + 1) % LCD_PAGE_MAX;
                    ESP_LOGI(TAG_MAIN, "Page -> %d", (int)g_sys.lcd_page);
                }
            } else if (evt.evt == KEY_EVT_LONG) {
                if (g_sys.sys_mode == MODE_NORMAL) {
                    g_sys.sys_mode = MODE_TH_CONFIG;
                    g_sys.lcd_page = LCD_PAGE_THRESHOLD;
                    ESP_LOGI(TAG_MAIN, "Enter threshold config");
                } else {
                    g_sys.sys_mode = MODE_NORMAL;
                    ESP_LOGI(TAG_MAIN, "Exit threshold config");
                }
            }
        }

        /* K3 (非配置模式): 蜂鸣器开关测试 */
        if (evt.key_id == KEY3_PRESSED && g_sys.sys_mode != MODE_TH_CONFIG) {
            if (evt.evt == KEY_EVT_SHORT) {
                if (g_sys.buzzer_mode == BUZZER_AUTO) {
                    g_sys.buzzer_mode = BUZZER_ON;
                    beep_on();
                    g_sys.buzzer_state = true;
                    ESP_LOGI(TAG_MAIN, "*** Buzzer TEST ON ***");
                } else {
                    g_sys.buzzer_mode = BUZZER_AUTO;
                    beep_off();
                    g_sys.buzzer_state = false;
                    ESP_LOGI(TAG_MAIN, "*** Buzzer TEST OFF ***");
                }
            }
        }

        /* K2: 阈值减小 (配置模式) */
        if (evt.key_id == KEY2_PRESSED && g_sys.sys_mode == MODE_TH_CONFIG) {
            if (evt.evt == KEY_EVT_SHORT) {
                if (g_sys.th_low > 50)       g_sys.th_low   -= 50;
                if (g_sys.th_mid_l > g_sys.th_low + 100) g_sys.th_mid_l -= 50;
                if (g_sys.th_mid_h > g_sys.th_mid_l + 100) g_sys.th_mid_h -= 50;
                if (g_sys.th_high > g_sys.th_mid_h + 100) g_sys.th_high -= 50;
            } else {
                if (g_sys.th_low > 200)       g_sys.th_low   -= 200;
                else                           g_sys.th_low    = 0;
                if (g_sys.th_mid_l > g_sys.th_low + 300) g_sys.th_mid_l -= 200;
                if (g_sys.th_mid_h > g_sys.th_mid_l + 300) g_sys.th_mid_h -= 200;
                if (g_sys.th_high > g_sys.th_mid_h + 300) g_sys.th_high -= 200;
            }
            ESP_LOGI(TAG_MAIN, "TH: H=%u MH=%u ML=%u L=%u",
                     g_sys.th_high, g_sys.th_mid_h, g_sys.th_mid_l, g_sys.th_low);
        }

        /* K3: 阈值增大 (配置模式) */
        if (evt.key_id == KEY3_PRESSED && g_sys.sys_mode == MODE_TH_CONFIG) {
            if (evt.evt == KEY_EVT_SHORT) {
                if (g_sys.th_high < 3550)     g_sys.th_high  += 50;
                if (g_sys.th_mid_h < g_sys.th_high - 100) g_sys.th_mid_h += 50;
                if (g_sys.th_mid_l < g_sys.th_mid_h - 100) g_sys.th_mid_l += 50;
                if (g_sys.th_low < g_sys.th_mid_l - 100) g_sys.th_low += 50;
            } else {
                if (g_sys.th_high < 3200)     g_sys.th_high  += 200;
                else                           g_sys.th_high   = 3600;
                if (g_sys.th_mid_h < g_sys.th_high - 300) g_sys.th_mid_h += 200;
                if (g_sys.th_mid_l < g_sys.th_mid_h - 300) g_sys.th_mid_l += 200;
                if (g_sys.th_low < g_sys.th_mid_l - 300) g_sys.th_low += 200;
            }
            ESP_LOGI(TAG_MAIN, "TH: H=%u MH=%u ML=%u L=%u",
                     g_sys.th_high, g_sys.th_mid_h, g_sys.th_mid_l, g_sys.th_low);
        }

        xSemaphoreGive(g_state_mutex);
    }
}

/* ============================================================
 *  Task 4: LCD 显示刷新任务 (300ms 周期)
 * ============================================================ */
static void task_lcd(void *pvParameters)
{
    ESP_LOGI(TAG_MAIN, "LCD task started");
    /* 缓存上次显示状态，仅变化时刷新，消除闪烁 */
    sys_state_t last = {0};
    memset(&last, 0xFF, sizeof(last)); /* 强制首次全量刷新 */

    for (;;) {
        sys_state_t local;
        if (xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            memcpy(&local, &g_sys, sizeof(local));
            xSemaphoreGive(g_state_mutex);
        } else {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        /* 检测是否有实质性变化需要刷新 LCD */
        bool need_refresh = false;
        if (local.lcd_page != last.lcd_page ||
            local.sys_mode != last.sys_mode ||
            local.light_level != last.light_level ||
            local.buzzer_state != last.buzzer_state ||
            local.buzzer_mode != last.buzzer_mode ||
            local.wifi_connected != last.wifi_connected ||
            local.tcp_client_fd != last.tcp_client_fd ||
            strcmp(local.ip_addr, last.ip_addr) != 0 ||
            abs((int)local.light_filtered - (int)last.light_filtered) > 30 ||
            local.th_high != last.th_high ||
            local.th_mid_h != last.th_mid_h ||
            local.th_mid_l != last.th_mid_l ||
            local.th_low != last.th_low) {
            need_refresh = true;
        }

        if (need_refresh) {
            lcd_update(&local);
            memcpy(&last, &local, sizeof(last));
        }

        vTaskDelay(pdMS_TO_TICKS(500)); /* 500ms 刷新周期 */
    }
}

/* ============================================================
 *  Task 5: 串口接收任务
 * ============================================================ */
static void task_uart_rx(void *pvParameters)
{
    ESP_LOGI(TAG_MAIN, "UART RX task started");
    uint8_t rx_buf[512];
    char resp[TX_MSG_MAX_LEN];

    for (;;) {
        int len = uart_read_bytes(UART_NUM, rx_buf, sizeof(rx_buf) - 1,
                                   pdMS_TO_TICKS(100));
        if (len <= 0) continue;

        rx_buf[len] = '\0';
        ESP_LOGI(TAG_MAIN, "UART RX: %s", (char *)rx_buf);

        if (xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            int ret = protocol_parse_downlink((const char *)rx_buf, &g_sys,
                                              resp, sizeof(resp));
            xSemaphoreGive(g_state_mutex);

            if (ret >= 0) {
                uart_write_bytes(UART_NUM, resp, strlen(resp));
                uart_write_bytes(UART_NUM, "\n", 1);
                send_to_tcp(resp);
                xEventGroupSetBits(g_evt, EVT_CMD_RECEIVED);
            } else if (resp[0] != '\0') {
                uart_write_bytes(UART_NUM, resp, strlen(resp));
                uart_write_bytes(UART_NUM, "\n", 1);
            }
        }
    }
}

/* ============================================================
 *  Task 6: 串口发送任务
 * ============================================================ */
static void task_uart_tx(void *pvParameters)
{
    ESP_LOGI(TAG_MAIN, "UART TX task started");
    char *msg;

    for (;;) {
        if (xQueueReceive(g_uart_tx_queue, &msg, pdMS_TO_TICKS(500)) == pdTRUE) {
            if (msg) {
                uart_write_bytes(UART_NUM, msg, strlen(msg));
                uart_write_bytes(UART_NUM, "\r\n", 2);
                free(msg);
            }
        }
    }
}

/* ============================================================
 *  Task 7: TCP 服务器任务
 * ============================================================ */
static void task_tcp_server(void *pvParameters)
{
    ESP_LOGI(TAG_MAIN, "TCP server task started");

    xEventGroupWaitBits(g_evt, EVT_WIFI_READY, pdFALSE, pdFALSE, portMAX_DELAY);

    int listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_fd < 0) {
        ESP_LOGE(TAG_MAIN, "socket() failed: %d", errno);
        vTaskDelete(NULL);
        return;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(TCP_SERVER_PORT);

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0 ||
        listen(listen_fd, 3) < 0) {
        ESP_LOGE(TAG_MAIN, "bind/listen failed: %d", errno);
        close(listen_fd);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG_MAIN, "TCP server listening on port %d", TCP_SERVER_PORT);

    int flags = fcntl(listen_fd, F_GETFL, 0);
    fcntl(listen_fd, F_SETFL, flags | O_NONBLOCK);

    fd_set read_fds;
    int client_fd = -1;
    char rx_buf[512];
    char resp[TX_MSG_MAX_LEN];
    char *tcp_msg = NULL;

    for (;;) {
        FD_ZERO(&read_fds);
        FD_SET(listen_fd, &read_fds);
        int max_fd = listen_fd;

        if (client_fd >= 0) {
            FD_SET(client_fd, &read_fds);
            if (client_fd > max_fd) max_fd = client_fd;
        }

        struct timeval tv = { .tv_sec = 0, .tv_usec = 200000 };
        int ret = select(max_fd + 1, &read_fds, NULL, NULL, &tv);

        if (ret < 0) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        /* 新连接 */
        if (FD_ISSET(listen_fd, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int new_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
            if (new_fd >= 0) {
                if (client_fd >= 0) close(client_fd);
                client_fd = new_fd;
                if (xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    g_sys.tcp_client_fd = client_fd;
                    xSemaphoreGive(g_state_mutex);
                }
                ESP_LOGI(TAG_MAIN, "TCP client connected: %s",
                         inet_ntoa(client_addr.sin_addr));
            }
        }

        /* 接收数据 */
        if (client_fd >= 0 && FD_ISSET(client_fd, &read_fds)) {
            int n = recv(client_fd, rx_buf, sizeof(rx_buf) - 1, 0);
            if (n > 0) {
                rx_buf[n] = '\0';
                ESP_LOGI(TAG_MAIN, "TCP RX: %s", rx_buf);
                if (xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    int pr = protocol_parse_downlink(rx_buf, &g_sys, resp, sizeof(resp));
                    xSemaphoreGive(g_state_mutex);
                    if (pr >= 0) {
                        send(client_fd, resp, strlen(resp), 0);
                        send(client_fd, "\n", 1, 0);
                        send_to_uart(resp);
                        xEventGroupSetBits(g_evt, EVT_CMD_RECEIVED);
                    } else if (resp[0] != '\0') {
                        send(client_fd, resp, strlen(resp), 0);
                        send(client_fd, "\n", 1, 0);
                    }
                }
            } else if (n == 0) {
                ESP_LOGI(TAG_MAIN, "TCP client disconnected");
                close(client_fd);
                client_fd = -1;
                if (xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    g_sys.tcp_client_fd = -1;
                    xSemaphoreGive(g_state_mutex);
                }
            }
        }

        /* 发送队列 */
        while (client_fd >= 0 &&
               xQueueReceive(g_tcp_tx_queue, &tcp_msg, 0) == pdTRUE) {
            if (tcp_msg) {
                send(client_fd, tcp_msg, strlen(tcp_msg), 0);
                send(client_fd, "\r\n", 2, 0);
                free(tcp_msg);
            }
        }
    }

    if (client_fd >= 0) close(client_fd);
    close(listen_fd);
}

/* ============================================================
 *  WiFi 初始化任务
 * ============================================================ */
static void task_wifi_init(void *pvParameters)
{
    ESP_LOGI(TAG_MAIN, "WiFi init task started");

    wifi_sta_init();  /* 内部已完成 netif / event loop 初始化 */

    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif) {
        esp_netif_ip_info_t ip_info;
        if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
            if (xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
                snprintf(g_sys.ip_addr, sizeof(g_sys.ip_addr),
                         IPSTR, IP2STR(&ip_info.ip));
                g_sys.wifi_connected = true;
                xSemaphoreGive(g_state_mutex);
                ESP_LOGI(TAG_MAIN, "WiFi connected, IP: %s", g_sys.ip_addr);
            }
            xEventGroupSetBits(g_evt, EVT_WIFI_READY);
            vTaskDelete(NULL);
            return;
        }
    }

    if (xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        snprintf(g_sys.ip_addr, sizeof(g_sys.ip_addr), "0.0.0.0");
        g_sys.wifi_connected = false;
        xSemaphoreGive(g_state_mutex);
    }
    vTaskDelete(NULL);
}

/* ============================================================
 *  系统初始化
 * ============================================================ */
static void system_init(void)
{
    esp_err_t ret;

    /* ---- 1. NVS ---- */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* ---- 2. 全局状态默认值 ---- */
    memset(&g_sys, 0, sizeof(g_sys));
    g_sys.th_high       = DEFAULT_TH_HIGH;
    g_sys.th_mid_h      = DEFAULT_TH_MID_H;
    g_sys.th_mid_l      = DEFAULT_TH_MID_L;
    g_sys.th_low        = DEFAULT_TH_LOW;
    g_sys.lcd_page      = LCD_PAGE_DATA;
    g_sys.sys_mode      = MODE_NORMAL;
    g_sys.buzzer_mode   = BUZZER_AUTO;
    g_sys.buzzer_state  = false;
    g_sys.wifi_connected = false;
    g_sys.uart_enabled  = true;
    g_sys.tcp_client_fd = -1;
    g_sys.light_filtered = 1500;  /* 启动默认中位值，避免误触发蜂鸣器 */
    g_sys.light_level    = LIGHT_LEVEL_NORMAL;
    snprintf(g_sys.ip_addr, sizeof(g_sys.ip_addr), "0.0.0.0");

    /* ---- 3. 同步对象 ---- */
    g_state_mutex   = xSemaphoreCreateMutex();
    g_lcd_mutex     = xSemaphoreCreateMutex();
    g_uart_tx_queue = xQueueCreate(16, sizeof(char *));
    g_tcp_tx_queue  = xQueueCreate(16, sizeof(char *));
    g_cmd_queue     = xQueueCreate(8, sizeof(char *));
    g_key_queue     = xQueueCreate(8, sizeof(key_event_t));
    g_evt           = xEventGroupCreate();

    /* ---- 4. 硬件驱动初始化 (顺序敏感) ---- */
    ESP_LOGI(TAG_MAIN, "Initializing hardware...");

    beep_init();
    beep_off();

    /* 启动自检: 蜂鸣器响 100ms 两次，验证硬件正常 */
    ESP_LOGI(TAG_MAIN, "Buzzer self-test...");
    beep_on();  vTaskDelay(pdMS_TO_TICKS(100));
    beep_off(); vTaskDelay(pdMS_TO_TICKS(100));
    beep_on();  vTaskDelay(pdMS_TO_TICKS(100));
    beep_off();

    Key_Init();
    key_ctx_init();

    lcd_init();
    lcd_clear(BLACK);
    lcd_on();

    uart_init();
    adc_single_short_init();
    led_init();
    gptimer_init();

    led_on();

    ESP_LOGI(TAG_MAIN, "System init complete");
}

/* ============================================================
 *  主入口
 * ============================================================ */
void app_main(void)
{
    ESP_LOGI(TAG_MAIN, "============================================");
    ESP_LOGI(TAG_MAIN, " Light Monitor System v1.1 — ESP32-S3");
    ESP_LOGI(TAG_MAIN, "============================================");

    system_init();

    /* 创建任务 (高优先级先创建) */
    xTaskCreate(task_wifi_init,   "wifi",   4096, NULL, 4, NULL);
    xTaskCreate(task_sensor,      "sensor", 4096, NULL, 3, NULL);
    xTaskCreate(task_control,     "control",4096, NULL, 3, NULL);
    xTaskCreate(task_key,         "key",    3072, NULL, 2, NULL);
    xTaskCreate(task_key_handler, "keyhdr", 3072, NULL, 2, NULL);
    xTaskCreate(task_lcd,         "lcd",    4096, NULL, 2, NULL);
    xTaskCreate(task_uart_rx,     "uart_rx",4096, NULL, 3, NULL);
    xTaskCreate(task_uart_tx,     "uart_tx",3072, NULL, 2, NULL);
    xTaskCreate(task_tcp_server,  "tcp",    6144, NULL, 4, NULL);

    ESP_LOGI(TAG_MAIN, "All tasks created — system running");

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
