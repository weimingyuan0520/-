#ifndef __KEY_H
#define __KEY_H
#define KEY1_GPIO_NUM GPIO_NUM_15    // K1 — 模式切换
#define KEY2_GPIO_NUM GPIO_NUM_17    // K2 — 阈值-/关继电器
#define KEY3_GPIO_NUM GPIO_NUM_8     // K3 — 阈值+/开继电器
#define KEY4_GPIO_NUM GPIO_NUM_40    // 未用

#define KEY1_PRESSED 1
#define KEY2_PRESSED 2
#define KEY3_PRESSED 3
#define KEY4_PRESSED 4


void Key_Init(void);
uint8_t Key_Scan(void);
#endif /* __KEY_H */
