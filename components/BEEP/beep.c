#include <stdio.h>
#include "beep.h"
#include "driver/gpio.h"

void beep_init(void)
{
    gpio_config_t io_conf;
    esp_err_t esp_err;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << BEEP_GPIO_NUM);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;

    esp_err = gpio_config(&io_conf);
    
    if (esp_err != ESP_OK) {
        printf("gpio_config failed with error 0x%x\n", esp_err);
        return;
    }
}
void beep_on(void)
{
    gpio_set_level(BEEP_GPIO_NUM, ON);
}
void beep_off(void)
{
    gpio_set_level(BEEP_GPIO_NUM, OFF);
}
