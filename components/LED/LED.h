#ifndef __LED_H
#define __LED_H
#define LED_GPIO_NUM GPIO_NUM_2     // 普中 ESP32-S3 开发板自带红色 LED
#define ON 1
#define OFF 0
void led_init(void);
void led_on(void);
void led_off(void);
void led_toggle(void);
void led_set(uint8_t status);
#endif