#include "gptim.h"
#include "driver/gptimer.h"
#include "esp_attr.h"

gptimer_handle_t gptimer = NULL;   //  gptimer句柄
uint8_t flag_timer = 0 ; 
bool IRAM_ATTR TimerCallback (gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx);

void gptimer_init(void)
{
    gptimer_config_t    gptimer_structure = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,    // 时钟源
        .direction = GPTIMER_COUNT_UP,       //计数方向，递增方向       
        .flags.intr_shared = 0,               // 中断不共享
        .intr_priority = 0,                   //中断优先级
        .resolution_hz = 1000000,              // 计数分辨率，1Mhz，每个步长=1/resolution 秒
    };
    gptimer_new_timer(&gptimer_structure, &gptimer);    // 1.创建gptimer

    gptimer_alarm_config_t  alarm_structure = {
        .alarm_count = 500000,    // 计数目标值，此处一次溢出时间为：500000* 1/1000000=500ms
        .flags.auto_reload_on_alarm = 1,   //自动重载
        .reload_count  = 0,    //重载值，为0时使用alarm_count
    };
    gptimer_set_alarm_action(gptimer, &alarm_structure);  // 2.设置gptimer的报警动作

    gptimer_event_callbacks_t   callbacks_structure = {
        .on_alarm = TimerCallback,
    };
    gptimer_register_event_callbacks(gptimer, &callbacks_structure, NULL); // 3.注册gptimer事件回调函数

    gptimer_enable(gptimer);   // 4.使能gptimer
    gptimer_start(gptimer);    // 5.启动gptimer
}
//中断回调函数，当gptimer溢出时，会调用此函数
bool IRAM_ATTR TimerCallback (gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    flag_timer = 1 ;
    return 0 ;
}
