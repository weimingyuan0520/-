#ifndef __CAMERA_H__
#define __CAMERA_H__


#include <stdint.h>

/* 引脚声明 */
#define CAM_PIN_PWDN     -1   //未使用
#define CAM_PIN_RESET    -1   //未使用
#define CAM_PIN_XCLK     GPIO_NUM_15  //摄像头时钟震荡来源
#define CAM_PIN_SIOD     GPIO_NUM_4   //SCCB接口-数据线
#define CAM_PIN_SIOC     GPIO_NUM_5  //SCCB接口-时钟线
#define CAM_PIN_D7       GPIO_NUM_16  //DVP接口-数据信号 D0~D7
#define CAM_PIN_D6       GPIO_NUM_17
#define CAM_PIN_D5       GPIO_NUM_18
#define CAM_PIN_D4       GPIO_NUM_12
#define CAM_PIN_D3       GPIO_NUM_10
#define CAM_PIN_D2       GPIO_NUM_8
#define CAM_PIN_D1       GPIO_NUM_9
#define CAM_PIN_D0       GPIO_NUM_11
#define CAM_PIN_VSYNC    GPIO_NUM_6  //DVP接口-帧同步信号
#define CAM_PIN_HREF     GPIO_NUM_7  //DVP接口-行同步信号
#define CAM_PIN_PCLK     GPIO_NUM_13  //DVP接口-时钟信号

#define CAM_PWDN(x)     x ? gpio_set_level(CAM_PIN_PWDN, 1)  : gpio_set_level(CAM_PIN_PWDN, 0) 
#define CAM_RST(x)      x ? gpio_set_level(CAM_PIN_RESET, 1) : gpio_set_level(CAM_PIN_RESET, 0) 

/* 函数声明 */
void camera_init(void);                 /* 摄像头初始化 */
void camera_show(uint16_t x, uint16_t y);  /* 摄像头显示 */
#endif
