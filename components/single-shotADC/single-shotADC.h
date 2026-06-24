#ifndef __SHADC_H_
#define __SHADC_H

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"

extern adc_oneshot_unit_handle_t adc_unit_handle ;
extern adc_cali_handle_t          adc_cali_handle ;

void adc_single_short_init(void);
int  adc_read_light_sensor(void);   /* 读取光敏传感器 (已校准) */

#endif
