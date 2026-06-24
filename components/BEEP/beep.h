#ifndef __BEEP_H
#define __BEEP_H
#define BEEP_GPIO_NUM GPIO_NUM_6
#define ON 1
#define OFF 0
void beep_init(void);
void beep_on(void);
void beep_off(void);

#endif