#ifndef __PWM_H_
#define __PWM_H_

#include <stdint.h>
#define PWM_NUM  GPIO_NUM_4
#define PWM_FREQ 50

void pwm_init(void);
void duty_set(uint16_t duty);

#endif
