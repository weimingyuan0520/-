#include <stdio.h>
#include "rgb_led.h"
#include "driver/gpio.h"

void rgb_led_init(void)
{
    gpio_config_t io_conf;
    esp_err_t esp_err;
    io_conf.intr_type = GPIO_INTR_DISABLE; // disable interrupt
    io_conf.mode = GPIO_MODE_INPUT_OUTPUT; // output and input mode
    io_conf.pin_bit_mask = (1ULL << LED_R_GPIO_NUM | 1ULL << LED_G_GPIO_NUM | 1ULL << LED_B_GPIO_NUM); // bit mask of the pins that you want to set
    io_conf.pull_down_en = 0; // disable pull-down mode
    io_conf.pull_up_en = 0; // disable pull-up mode

    esp_err = gpio_config(&io_conf);
    
    if (esp_err != ESP_OK) {
        printf("gpio_config failed with error 0x%x\n", esp_err);
        return;
    }
}
// ledcolor: 1:红 2:绿 3:蓝
void rgb_led_on(uint8_t ledcolor)
{
    if(ledcolor == 1)
    {
        gpio_set_level(LED_R_GPIO_NUM, ON);
    }
    else if(ledcolor == 2)
    {
        gpio_set_level(LED_G_GPIO_NUM, ON);
    }
    else if(ledcolor == 3)
    {
        gpio_set_level(LED_B_GPIO_NUM, ON);
    }
}
void rgb_led_off(uint8_t ledcolor)
{
    if(ledcolor == 1)
    {
        gpio_set_level(LED_R_GPIO_NUM, OFF);
    }
    else if(ledcolor == 2)
    {
        gpio_set_level(LED_G_GPIO_NUM, OFF);
    }
    else if(ledcolor == 3)
    {
        gpio_set_level(LED_B_GPIO_NUM, OFF);
    }
}

// toggle led 状态反转
void rgb_led_toggle(uint8_t ledcolor)
{
    if(ledcolor == 1)
    {
        gpio_set_level(LED_R_GPIO_NUM, !gpio_get_level(LED_R_GPIO_NUM));
    }
    else if(ledcolor == 2)
    {
        gpio_set_level(LED_G_GPIO_NUM, !gpio_get_level(LED_G_GPIO_NUM));
    }
    else if(ledcolor == 3)
    {
        gpio_set_level(LED_B_GPIO_NUM, !gpio_get_level(LED_B_GPIO_NUM));
    }
}
