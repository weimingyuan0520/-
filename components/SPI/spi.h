/***
 * 接线说明：TFTLCD模块-->ESP32 IO
 *          (GND)-->(GND)
            (VCC)-->(3V3)
            (SCL)-->(21)
            (SDI)-->(47)
            (RST)-->(EN)
            (RS)-->(41)
            (BL)-->(14)
            (SDO)-->(48)
            (CS)-->(42)
            (TP_CS)-->(46)
            (TP_PEN)-->(2)
***/

#ifndef __MYSPI_H_
#define __MYSPI_H_

#include "driver/spi_master.h"

extern spi_device_handle_t spi2_handle;

#define SPI_MISO_PIN     GPIO_NUM_48    //MISO 引脚
#define SPI_MOSI_PIN     GPIO_NUM_47    //MOSI 引脚
#define SPI_SCLK_PIN     GPIO_NUM_21    //SCLK 引脚

void spi2_init(void);
uint8_t spi2_transfer_byte(uint8_t data);
void spi2_write_data(uint8_t *data, int len);
#endif
