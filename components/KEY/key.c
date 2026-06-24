#include <stdio.h>
#include "key.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
void Key_Init(void)
{
    gpio_config_t io_conf;
    esp_err_t esp_err;
    io_conf.intr_type = GPIO_INTR_DISABLE; //禁用中断
    io_conf.mode = GPIO_MODE_INPUT;  //输入模式
    io_conf.pin_bit_mask = (1ULL << KEY1_GPIO_NUM | 1ULL << KEY2_GPIO_NUM | 
        1ULL << KEY3_GPIO_NUM | 1ULL << KEY4_GPIO_NUM); //选择按键对用的GPIO
    io_conf.pull_down_en = 0; //不使用下拉
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;  //使用上拉

    esp_err = gpio_config(&io_conf); //配置GPIO
    
    if (esp_err != ESP_OK) {
        printf("gpio_config failed with error 0x%x\n", esp_err);
        return;
    }
}

uint8_t Key_Scan(void)
{
    uint8_t key_num = 0; //按键值
    if (gpio_get_level(KEY1_GPIO_NUM) == 0) {
        vTaskDelay(20); //按下消抖
        while(gpio_get_level(KEY1_GPIO_NUM) == 0); //等待按键释放
        vTaskDelay(20); //弹起消抖
        key_num = KEY1_PRESSED;
    }
    if (gpio_get_level(KEY2_GPIO_NUM) == 0) {
        vTaskDelay(20); //按下消抖
        while(gpio_get_level(KEY2_GPIO_NUM) == 0); //等待按键释放
        vTaskDelay(20); //弹起消抖
        key_num = KEY2_PRESSED;
    }
    if (gpio_get_level(KEY3_GPIO_NUM) == 0) {
        vTaskDelay(20); //按下消抖
        while(gpio_get_level(KEY3_GPIO_NUM) == 0); //等待按键释放
        vTaskDelay(20); //弹起消抖
        key_num = KEY3_PRESSED;
    }
    if (gpio_get_level(KEY4_GPIO_NUM) == 0) {
        vTaskDelay(20); //按下消抖
        while(gpio_get_level(KEY4_GPIO_NUM) == 0); //等待按键释放
        vTaskDelay(20); //弹起消抖
        key_num = KEY4_PRESSED;
    }
    return key_num;      
}