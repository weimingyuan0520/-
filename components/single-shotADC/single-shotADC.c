#include <stdio.h>
#include "single-shotADC.h"
#include "driver/adc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

adc_oneshot_unit_handle_t adc_unit_handle ;
adc_cali_handle_t   adc_cali_handle = NULL;

void adc_single_short_init(void)
{
    esp_err_t ret;

    /* ---- 1. 创建 ADC oneshot 单元 ---- */
    adc_oneshot_unit_init_cfg_t oneshot_structure = {
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
        .unit_id = ADC_UNIT_1,
    };
    ret = adc_oneshot_new_unit(&oneshot_structure, &adc_unit_handle);
    if (ret != ESP_OK) {
        printf("[ADC] FATAL: adc_oneshot_new_unit failed! err=0x%x\n", ret);
        adc_unit_handle = NULL;
        return;
    }

    /* ---- 2. 配置 ADC 通道 (CH3=GPIO4可调电阻, CH4=GPIO5光敏) ---- */
    adc_oneshot_chan_cfg_t oneshot_chan_structure = {
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ret = adc_oneshot_config_channel(adc_unit_handle, ADC_CHANNEL_3, &oneshot_chan_structure);
    if (ret != ESP_OK) {
        printf("[ADC] WARN: config CH3 failed, err=0x%x\n", ret);
    }
    ret = adc_oneshot_config_channel(adc_unit_handle, ADC_CHANNEL_4, &oneshot_chan_structure);
    if (ret != ESP_OK) {
        printf("[ADC] WARN: config CH4 failed, err=0x%x\n", ret);
    }

    /* ---- 3. ADC 校准 (ESP32-S3 内置曲线拟合校准) ---- */
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id  = ADC_UNIT_1,
        .chan     = ADC_CHANNEL_4,
        .atten    = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle);
    if (ret == ESP_OK) {
        printf("[ADC] Calibration scheme created (curve fitting)\n");
    } else {
        printf("[ADC] WARN: Calibration not available, will use raw values (err=0x%x)\n", ret);
        adc_cali_handle = NULL;
    }

    /* ---- 4. 丢弃前几次读数使 ADC 电路稳定 ---- */
    int dummy;
    for (int i = 0; i < 5; i++) {
        adc_oneshot_read(adc_unit_handle, ADC_CHANNEL_4, &dummy);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    printf("[ADC] Init done, initial raw=%d\n", dummy);
}

/* 读取光敏传感器原始值并返回校准后的毫伏值 (0-3600mV) */
int adc_read_light_sensor(void)
{
    int raw = 0;
    int voltage_mv = 0;

    if (adc_unit_handle == NULL) {
        return -1;
    }

    if (adc_oneshot_read(adc_unit_handle, ADC_CHANNEL_4, &raw) != ESP_OK) {
        return -1;
    }

    if (adc_cali_handle != NULL) {
        if (adc_cali_raw_to_voltage(adc_cali_handle, raw, &voltage_mv) != ESP_OK) {
            voltage_mv = raw * 3600 / 4095;  /* 无校准回退：原始值→mV */
        }
    } else {
        voltage_mv = raw * 3600 / 4095;
    }

    return voltage_mv;  /* 返回 0-3600 mV */
}