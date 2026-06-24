#ifndef __RELAY_H
#define __RELAY_H
#define RELAY_GPIO_NUM GPIO_NUM_10   // 继电器控制
#define ON 1
#define OFF 0
void relay_init(void);
void relay_on(void);
void relay_off(void);

#endif