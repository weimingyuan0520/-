#ifndef __RGB_LED_H
#define __RGB_LED_H

#define LED_R_GPIO_NUM GPIO_NUM_5
#define LED_G_GPIO_NUM GPIO_NUM_7
#define LED_B_GPIO_NUM GPIO_NUM_16


#define ON 1
#define OFF 0
void rgb_led_init(void);

// ledcolor: 1:红 2:绿 3:蓝
void rgb_led_on(uint8_t ledcolor);
void rgb_led_off(uint8_t ledcolor);
void rgb_led_toggle(uint8_t ledcolor);

#endif