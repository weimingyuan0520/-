/**
 * @file    filter.h
 * @brief   滑动平均数字滤波器 — 用于光敏传感器 ADC 数据平滑
 * @note    窗口大小可配置 (默认 FILTER_WINDOW_SIZE=8)
 */

#ifndef __FILTER_H_
#define __FILTER_H_

#include <stdint.h>

#define FILTER_WINDOW_SIZE  8       /* 滑动窗口大小 */

/**
 * @brief 滑动平均滤波器句柄
 */
typedef struct {
    uint16_t buf[FILTER_WINDOW_SIZE];  /* 环形缓冲区 */
    uint8_t  index;                     /* 当前写入位置 */
    uint8_t  count;                     /* 已填充样本数 (启动阶段 < WINDOW_SIZE) */
    uint32_t sum;                       /* 当前窗口总和 (快速滑动) */
} sliding_filter_t;

/**
 * @brief 初始化滤波器
 */
void filter_init(sliding_filter_t *f);

/**
 * @brief 喂入新采样值并返回滤波结果
 * @param f    滤波器句柄
 * @param raw  原始 ADC 采样值 (mV)
 * @return     滤波后的值 (mV)
 */
uint16_t filter_update(sliding_filter_t *f, uint16_t raw);

/**
 * @brief 重置滤波器状态
 */
void filter_reset(sliding_filter_t *f);

#endif /* __FILTER_H_ */
