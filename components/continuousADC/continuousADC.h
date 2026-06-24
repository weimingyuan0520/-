#ifndef __CTADC_H_
#define __CTADC_H_

#include "driver/adc.h"
#include "hal/adc_types.h"
#include "esp_adc/adc_continuous.h"

extern uint8_t *data_value;
extern uint16_t adc_value_rp;
extern uint8_t adc_channelnum_rp;
extern uint16_t adc_value_rl;
extern uint8_t adc_channelnum_rl;
extern uint8_t flag_adc_conv ;
extern adc_continuous_handle_t adc_countinuous_handle ;

void adc_continuous_init(void);

#endif
