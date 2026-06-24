#include <stdio.h>
#include "LED.h"
#include "driver/gpio.h"

void led_init(void)
{
    gpio_config_t io_conf;
    esp_err_t esp_err;
    io_conf.intr_type = GPIO_INTR_DISABLE; // disable interrupt
    io_conf.mode = GPIO_MODE_INPUT_OUTPUT; // output and input mode
    io_conf.pin_bit_mask = (1ULL << LED_GPIO_NUM); // bit mask of the pins that you want to set
    io_conf.pull_down_en = 0; // disable pull-down mode
    io_conf.pull_up_en = 0; // disable pull-up mode

    esp_err = gpio_config(&io_conf);
    
    if (esp_err != ESP_OK) {
        printf("gpio_config failed with error 0x%x\n", esp_err);
        return;
    }
}
void led_on(void)
{
    gpio_set_level(LED_GPIO_NUM, ON);
}
void led_off(void)
{
    gpio_set_level(LED_GPIO_NUM, OFF);
}
// toggle led 状态反转
void led_toggle(void)
{    
    gpio_set_level(LED_GPIO_NUM, !gpio_get_level(LED_GPIO_NUM));
}

void led_set(uint8_t status)
{
    gpio_set_level(LED_GPIO_NUM, status);
}
