/**
 * @file    filter.c
 * @brief   滑动平均滤波器实现
 */

#include "filter.h"
#include <string.h>

void filter_init(sliding_filter_t *f)
{
    memset(f->buf, 0, sizeof(f->buf));
    f->index = 0;
    f->count = 0;
    f->sum   = 0;
}

uint16_t filter_update(sliding_filter_t *f, uint16_t raw)
{
    if (f->count < FILTER_WINDOW_SIZE) {
        /* ---- 启动阶段：直接累加，不丢弃旧值 ---- */
        f->buf[f->index] = raw;
        f->sum += raw;
        f->index++;
        f->count++;
        return (uint16_t)(f->sum / f->count);
    }

    /* ---- 稳态阶段：滑动窗口 ---- */
    f->sum -= f->buf[f->index];          /* 减去即将被覆盖的旧值 */
    f->buf[f->index] = raw;              /* 写入新值 */
    f->sum += raw;                       /* 累加新值 */
    f->index = (f->index + 1) % FILTER_WINDOW_SIZE;

    return (uint16_t)(f->sum / FILTER_WINDOW_SIZE);
}

void filter_reset(sliding_filter_t *f)
{
    filter_init(f);
}
