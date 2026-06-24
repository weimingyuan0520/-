#include "continuousADC.h"

uint8_t *data_value;
uint16_t adc_value_rp;
uint8_t adc_channelnum_rp;
uint16_t adc_value_rl;
uint8_t adc_channelnum_rl;
uint8_t flag_adc_conv = 0 ;

adc_continuous_handle_t adc_countinuous_handle ;

bool continuous_callback(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    data_value = edata->conv_frame_buffer;
    if(edata->size == 8)
    {
        adc_value_rp = ((data_value[1] & 0x0F) << 8) | data_value[0];
        adc_channelnum_rp = data_value[1] >> 5 ;
        adc_value_rl = ((data_value[5] & 0x0F) << 8) | data_value[4];
        adc_channelnum_rl = data_value[5] >> 5 ;
        return true;
    }
    return false ;
}

void adc_continuous_init(void)
{
    adc_continuous_handle_cfg_t adc_contunuous_structure = {
        .conv_frame_size = 8,
        .max_store_buf_size = 1024,
    };
    adc_continuous_new_handle(&adc_contunuous_structure, &adc_countinuous_handle);

    adc_digi_pattern_config_t adc_digi_arr[] = {
        {
            .atten = ADC_ATTEN_DB_11,       // 11dB衰减
            .bit_width = ADC_BITWIDTH_12,   // 输出12bit
            .channel = ADC_CHANNEL_3,       // 通道0
            .unit = ADC_UNIT_1              // ADC1
        },
        {
            .atten = ADC_ATTEN_DB_11,       // 11dB衰减
            .bit_width = ADC_BITWIDTH_12,   // 输出12bit
            .channel = ADC_CHANNEL_4,       // 通道0
            .unit = ADC_UNIT_1              // ADC1
        }
    };

    adc_continuous_config_t continuous_config_structure = {
        .adc_pattern = adc_digi_arr,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
        .pattern_num = 2,
        .sample_freq_hz = 20000,
    };
    adc_continuous_config(adc_countinuous_handle, &continuous_config_structure);

    adc_continuous_evt_cbs_t evt_stucture = {
        .on_conv_done = continuous_callback,
    };
    adc_continuous_register_event_callbacks(adc_countinuous_handle, &evt_stucture, NULL);

    adc_continuous_start(adc_countinuous_handle);            
}