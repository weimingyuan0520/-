#include <stdio.h>
#include "sensor-do.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
void Sensor_Do_Init(void)
{
    gpio_config_t io_conf;
    esp_err_t esp_err;
    io_conf.intr_type = GPIO_INTR_DISABLE; //禁用中断
    io_conf.mode = GPIO_MODE_INPUT;  //输入模式
    io_conf.pin_bit_mask = (1ULL << SENSOR_GPIO_NUM); //选择按键对用的GPIO
    io_conf.pull_down_en = 0; //不使用下拉
    io_conf.pull_up_en = 0;  //不使用上拉

    esp_err = gpio_config(&io_conf); //配置GPIO
    
    if (esp_err != ESP_OK) {
        printf("gpio_config failed with error 0x%x\n", esp_err);
        return;
    }
}

uint8_t Sensor_Do_Read(void)
{    
    if (gpio_get_level(SENSOR_GPIO_NUM) == 1) {
        return 1;
    }
    else{
        return 0;   
    }
       
}