/**
 ****************************************************************************************************
 * @file        dht11.h
 * @author      普中科技
 * @version     V1.0
 * @date        2024-03-01
 * @brief       DHT11驱动代码
 * @license     Copyright (c) 2024-2034, 深圳市普中科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:普中科技 ESP32-S3 开发板
 * 在线视频:https://space.bilibili.com/2146492485
 * 技术论坛:www.prechin.net
 * 公司网址:www.prechin.cn
 * 购买地址:
 *
 ****************************************************************************************************
 */

#ifndef __DHT11_H
#define __DHT11_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h" 

/* 引脚定义 */
#define DHT11_DQ_GPIO_PIN       GPIO_NUM_13

/* DHT11引脚高低电平枚举 */
typedef enum 
{
    DHT11_PIN_RESET = 0u,
    DHT11_PIN_SET
}DHT11_GPIO_PinState;

/* IO操作 */
#define DHT11_DQ_IN     gpio_get_level(DHT11_DQ_GPIO_PIN)   /* 数据端口输入 */

/* DHT11端口定义 */
#define DHT11_DQ_OUT(x) do{ x ?                                                 \
                            gpio_set_level(DHT11_DQ_GPIO_PIN, DHT11_PIN_SET) :  \
                            gpio_set_level(DHT11_DQ_GPIO_PIN, DHT11_PIN_RESET); \
                        }while(0)

/* 函数声明 */
void dht11_reset(void);                                 /* 复位DHT11 */
uint8_t dht11_init(void);                               /* 初始化DHT11 */
uint8_t dht11_check(void);                              /* 等待DHT11的回应 */
uint8_t dht11_read_data(float *temperature, float *humidity);   /* 读取温湿度 */

#endif