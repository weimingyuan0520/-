#include <stdio.h>
#include "exit.h"
#include "driver/gpio.h"
#include "led.h"

void KEY_IRQHandler(void *arg)
{
    uint32_t gpio_num = (uint32_t) arg;
 
    if (gpio_num == KEY1_GPIO_NUM || gpio_num == KEY2_GPIO_NUM || 
        gpio_num == KEY3_GPIO_NUM || gpio_num == KEY4_GPIO_NUM)
    {
        led_toggle();
    }   
   
}

void exit_init(void)
{
    gpio_config_t io_conf;
    
    io_conf.intr_type = GPIO_INTR_NEGEDGE; //下降沿触发
    io_conf.mode = GPIO_MODE_INPUT;  //输入模式
    io_conf.pin_bit_mask = (1ULL << KEY1_GPIO_NUM | 1ULL << KEY2_GPIO_NUM | 
        1ULL << KEY3_GPIO_NUM | 1ULL << KEY4_GPIO_NUM); //选择按键对用的GPIO
    io_conf.pull_down_en = 0; //不使用下拉
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;  //使用上拉

    gpio_config(&io_conf); //配置GPIO

    gpio_install_isr_service(ESP_INTR_FLAG_EDGE); //安装中断服务

    gpio_isr_handler_add(KEY1_GPIO_NUM, KEY_IRQHandler, (void *) KEY1_GPIO_NUM); //添加中断处理函数
    gpio_isr_handler_add(KEY2_GPIO_NUM, KEY_IRQHandler, (void *) KEY2_GPIO_NUM);
    gpio_isr_handler_add(KEY3_GPIO_NUM, KEY_IRQHandler, (void *) KEY3_GPIO_NUM);
    gpio_isr_handler_add(KEY4_GPIO_NUM, KEY_IRQHandler, (void *) KEY4_GPIO_NUM);

    gpio_intr_enable(KEY1_GPIO_NUM | KEY2_GPIO_NUM | KEY3_GPIO_NUM | KEY4_GPIO_NUM); //使能中断
}


