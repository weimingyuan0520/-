#include <stdio.h>
#include "myuart.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG_UART = "uart_drv";

void uart_init(void)
{
    esp_err_t ret;

    /* ---- 1. 配置串口参数 ---- */
    uart_config_t uart_structure = {
        .baud_rate  = UART_BAUDRATE,
        .data_bits  = UART_DATA_8_BITS,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .parity     = UART_PARITY_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_APB,
        .stop_bits  = UART_STOP_BITS_1,
    };
    ret = uart_param_config(UART_NUM, &uart_structure);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_UART, "uart_param_config failed! err=0x%x", ret);
        return;
    }

    /* ---- 2. 配置串口引脚 (TX=GPIO38, RX=GPIO36, 外接USB串口模块) ---- */
    ret = uart_set_pin(UART_NUM, UART_TX_GPIO_PIN, UART_RX_GPIO_PIN,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_UART, "uart_set_pin failed! err=0x%x", ret);
        return;
    }

    /* ---- 3. 安装串口驱动 (TX环形缓冲 + RX环形缓冲) ---- */
    ret = uart_driver_install(UART_NUM, RX_BUF_SIZE, RX_BUF_SIZE,
                              20, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_UART, "uart_driver_install failed! err=0x%x", ret);
        return;
    }

    ESP_LOGI(TAG_UART, "UART%d init OK: TX=GPIO%d, RX=GPIO%d, Baud=%d",
             UART_NUM, UART_TX_GPIO_PIN, UART_RX_GPIO_PIN, UART_BAUDRATE);
}