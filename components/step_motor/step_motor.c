
#include "step_motor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief       初始化步进电机
 * @param       无
 * @retval      无
 */
void step_motor_init(void)
{
    gpio_config_t gpio_init_struct = {0};

    gpio_init_struct.intr_type = GPIO_INTR_DISABLE;         /* 失能引脚中断 */
    gpio_init_struct.mode = GPIO_MODE_INPUT_OUTPUT;         /* 输入输出模式 */
    gpio_init_struct.pull_up_en = GPIO_PULLUP_ENABLE;      /* 使能上拉 */
    gpio_init_struct.pull_down_en = GPIO_PULLDOWN_DISABLE;   /* 失能下拉 */
    gpio_init_struct.pin_bit_mask = 1ull << INA_GPIO_PIN;   /* 设置的引脚的位掩码 */
    gpio_config(&gpio_init_struct);                         /* 配置GPIO */

    gpio_init_struct.pin_bit_mask = 1ull << INB_GPIO_PIN;   /* 设置的引脚的位掩码 */
    gpio_config(&gpio_init_struct);                         /* 配置GPIO */

    gpio_init_struct.pin_bit_mask = 1ull << INC_GPIO_PIN;   /* 设置的引脚的位掩码 */
    gpio_config(&gpio_init_struct);                         /* 配置GPIO */

    gpio_init_struct.pin_bit_mask = 1ull << IND_GPIO_PIN;   /* 设置的引脚的位掩码 */
    gpio_config(&gpio_init_struct);                         /* 配置GPIO */
}

/*******************************************************************************
* 函 数 名       : step_motor_run
* 函数功能		 : 输出一个数据给ULN2003从而实现向步进电机发送一个脉冲
	    		   步进角为5.625/64度，如果需要转动一圈，那么需要360/5.625*64=4096个脉冲信号，
				   假如需要转动90度，需要的脉冲数=90*4096/360=1024脉冲信号。
				   如果任意角度对应多少脉冲呢？脉冲=angle*64/(8*5.625)
				   正好对应下面计算公式
				   (64*angle/45)*8，这个angle就是转动90度，45表示5.625*8，8代表8个脉冲，for循环。
* 输    入       : step：指定步进控制节拍，可选值4或8
				   dir：方向选择,1：顺时针,0：逆时针
				   speed：速度1-10
				   angle：角度选择0-360
				   sta：电机运行状态，1启动，0停止
* 输    出    	 : 无
*******************************************************************************/
void step_motor_run(uint8_t step,uint8_t dir,uint8_t speed,uint16_t angle,uint8_t sta)
{
	char i=0;
	uint16_t j=0;

	if(sta==1)
	{
		if(dir==0)	//如果为逆时针旋转
		{
			for(j=0;j<64*angle/45;j++) 
			{
				for(i=0;i<8;i+=(8/step))
				{
					switch(i)//8个节拍控制：A->AB->B->BC->C->CD->D->DA
					{
						case 0: MOTOR_IN1(1);MOTOR_IN2(0);MOTOR_IN3(0);MOTOR_IN4(0);break;
						case 1: MOTOR_IN1(1);MOTOR_IN2(1);MOTOR_IN3(0);MOTOR_IN4(0);break;
						case 2: MOTOR_IN1(0);MOTOR_IN2(1);MOTOR_IN3(0);MOTOR_IN4(0);break;
						case 3: MOTOR_IN1(0);MOTOR_IN2(1);MOTOR_IN3(1);MOTOR_IN4(0);break;
						case 4: MOTOR_IN1(0);MOTOR_IN2(0);MOTOR_IN3(1);MOTOR_IN4(0);break;
						case 5: MOTOR_IN1(0);MOTOR_IN2(0);MOTOR_IN3(1);MOTOR_IN4(1);break;
						case 6: MOTOR_IN1(0);MOTOR_IN2(0);MOTOR_IN3(0);MOTOR_IN4(1);break;
						case 7: MOTOR_IN1(1);MOTOR_IN2(0);MOTOR_IN3(0);MOTOR_IN4(1);break;	
					}
					vTaskDelay(speed);		
				}	
			}
		}
		else	//如果为顺时针旋转
		{
			for(j=0;j<64*angle/45;j++)
			{
				for(i=0;i<8;i+=(8/step))
				{
					switch(i)//8个节拍控制：A->AB->B->BC->C->CD->D->DA
					{
						case 0: MOTOR_IN1(1);MOTOR_IN2(0);MOTOR_IN3(0);MOTOR_IN4(1);break;
						case 1: MOTOR_IN1(0);MOTOR_IN2(0);MOTOR_IN3(0);MOTOR_IN4(1);break;
						case 2: MOTOR_IN1(0);MOTOR_IN2(0);MOTOR_IN3(1);MOTOR_IN4(1);break;
						case 3: MOTOR_IN1(0);MOTOR_IN2(0);MOTOR_IN3(1);MOTOR_IN4(0);break;
						case 4: MOTOR_IN1(0);MOTOR_IN2(1);MOTOR_IN3(1);MOTOR_IN4(0);break;
						case 5: MOTOR_IN1(0);MOTOR_IN2(1);MOTOR_IN3(0);MOTOR_IN4(0);break;
						case 6: MOTOR_IN1(1);MOTOR_IN2(1);MOTOR_IN3(0);MOTOR_IN4(0);break;
						case 7: MOTOR_IN1(1);MOTOR_IN2(0);MOTOR_IN3(0);MOTOR_IN4(0);break;	
					}
					vTaskDelay(speed);		
				}	
			}	
		}		
	}
	else
	{
		MOTOR_IN1(0);MOTOR_IN2(0);MOTOR_IN3(0);MOTOR_IN4(0);	
	}		
}