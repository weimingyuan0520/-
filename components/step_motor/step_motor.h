
#ifndef _dc_motor_H
#define _dc_motor_H

#include "driver/gpio.h"

// 定义步进电机速度，值越小，速度越快
// 最小不能小于1
#define STEPMOTOR_MAXSPEED        1  
#define STEPMOTOR_MINSPEED        5 

/* 引脚定义 */
#define INA_GPIO_PIN    GPIO_NUM_9
#define INB_GPIO_PIN    GPIO_NUM_10 
#define INC_GPIO_PIN    GPIO_NUM_11 
#define IND_GPIO_PIN    GPIO_NUM_12  


/* 引脚的输出的电平状态 */
enum GPIO_OUTPUT_STATE
{
    PIN_RESET,
    PIN_SET
};

/* 端口操作 */
#define MOTOR_IN1(x)          do { x ?                                 \
                             gpio_set_level(INA_GPIO_PIN, PIN_SET) :  \
                             gpio_set_level(INA_GPIO_PIN, PIN_RESET); \
                            } while(0) 
#define MOTOR_IN2(x)          do { x ?                                 \
                             gpio_set_level(INB_GPIO_PIN, PIN_SET) :  \
                             gpio_set_level(INB_GPIO_PIN, PIN_RESET); \
                            } while(0)  
#define MOTOR_IN3(x)          do { x ?                                 \
                             gpio_set_level(INC_GPIO_PIN, PIN_SET) :  \
                             gpio_set_level(INC_GPIO_PIN, PIN_RESET); \
                            } while(0) 
#define MOTOR_IN4(x)          do { x ?                                 \
                             gpio_set_level(IND_GPIO_PIN, PIN_SET) :  \
                             gpio_set_level(IND_GPIO_PIN, PIN_RESET); \
                            } while(0)  


/* 函数声明*/
void step_motor_init(void);    /* 初始化 */
void step_motor_run(uint8_t step,uint8_t dir,uint8_t speed,uint16_t angle,uint8_t sta);

#endif
