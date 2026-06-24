#include "pwm.h"
#include "driver/ledc.h"

void pwm_init(void)
{
    ledc_timer_config_t ledctimer_structure = {
        .clk_cfg = LEDC_AUTO_CLK,   //自动选择时钟源
        .duty_resolution = LEDC_TIMER_10_BIT,  //10位分辨率，即0-1023
        .freq_hz = PWM_FREQ,             //频率
        .speed_mode = LEDC_LOW_SPEED_MODE,     //低速模式
        .timer_num = LEDC_TIMER_1,    //选择定时器1
    };
    ledc_timer_config(&ledctimer_structure);

    ledc_channel_config_t   ledcchannel_structure = {
        .channel = LEDC_CHANNEL_1,    //选择pwm通道1
        .duty = 512,   //占空比，范围是0~2^duty_resolution，结合duty_resolution，此处是0~1023
        .flags.output_invert = 0,    //输出不反转
        .gpio_num = PWM_NUM,         //选择pwm输出的GPIO引脚
        .hpoint = 0,    
        .intr_type = LEDC_INTR_DISABLE,  //中断类型，此处不使用中断
        .speed_mode = LEDC_LOW_SPEED_MODE,     //低速模式
        .timer_sel = LEDC_TIMER_1,    //选择定时器1
    };
    ledc_channel_config(&ledcchannel_structure);
}

void duty_set(uint16_t duty)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty);    //设置占空比
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);         //更新占空比
}