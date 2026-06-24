#ifndef __MYUART_H_
#define __MYUART_H_

/* 引脚和串口定义 */
#define UART_NUM UART_NUM_0             // 串口编号
#define UART_TX_GPIO_PIN GPIO_NUM_36    // TX — 接USB串口模块RX (已验证可用)
#define UART_RX_GPIO_PIN GPIO_NUM_38    // RX — 接USB串口模块TX
#define UART_BAUDRATE 115200            // 波特率
/* 串口接收相关定义 */
#define RX_BUF_SIZE 1024                // 环形缓冲区大小

void uart_init(void);

#endif
