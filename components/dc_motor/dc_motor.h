#ifndef __DC_MOTOR_H
#define __DC_MOTOR_H
#define DC_MOTOR_GPIO_NUM GPIO_NUM_4
#define Dc_Motor_ON 1
#define Dc_Motor_OFF 0
void Dc_Motor_init(void);
void Dc_Motor_on(void);
void Dc_Motor_off(void);

#endif