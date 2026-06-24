#include <stdio.h>
#include "dc_motor.h"
#include "driver/gpio.h"

void Dc_Motor_init(void)
{
    gpio_config_t io_conf;
    esp_err_t esp_err;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << DC_MOTOR_GPIO_NUM);
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = 0;

    esp_err = gpio_config(&io_conf);
    
    if (esp_err != ESP_OK) {
        printf("gpio_config failed with error 0x%x\n", esp_err);
        return;
    }
}
void Dc_Motor_on(void)
{
    gpio_set_level(DC_MOTOR_GPIO_NUM, Dc_Motor_ON);
}
void Dc_Motor_off(void)
{
    gpio_set_level(DC_MOTOR_GPIO_NUM, Dc_Motor_OFF);
}
