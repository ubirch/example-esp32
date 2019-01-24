//
// Created by wowa on 05.12.18.
//

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "../../esp-idf/components/esp_adc_cal/include/esp_adc_cal.h"

#include <driver/adc.h>

void init_adc() {

//    adc1_config_width();
//    adc1_config_channel_atten();

}

#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   1024          //Multisampling
#define NO_OF_MEAN      8
#define MEAN_SHIFT      3

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_0;     //GPIO34 if ADC1, GPIO14 if ADC2
static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;

static void check_efuse() {
    //Check TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        printf("eFuse Two Point: Supported\n");
    } else {
        printf("eFuse Two Point: NOT supported\n");
    }

    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        printf("eFuse Vref: Supported\n");
    } else {
        printf("eFuse Vref: NOT supported\n");
    }
}

static void print_char_val_type(esp_adc_cal_value_t val_type) {
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        printf("Characterized using Two Point Value\n");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        printf("Characterized using eFuse Vref\n");
    } else {
        printf("Characterized using Default Vref\n");
    }
}

void adc_task() {
    //Check if Two Point or Vref are burned into eFuse
    check_efuse();

    //Configure ADC
    if (unit == ADC_UNIT_1) {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(channel, atten);
        adc_set_clk_div(1);
    } else {
        adc2_config_channel_atten((adc2_channel_t) channel, atten);
        adc2_config_channel_atten(channel, atten);
        adc_set_clk_div(1);
    }

    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
    print_char_val_type(val_type);

    uint16_t adc_data[NO_OF_SAMPLES/NO_OF_MEAN];
    uint16_t adc_mean = 0;
    uint16_t adc_max = 0;


    //Continuously sample ADC1
    while (1) {
        uint32_t adc_reading = 0;
        //Multisampling
        for (int i = 0; i < NO_OF_SAMPLES/NO_OF_MEAN; i++) {
            if (unit == ADC_UNIT_1) {
                // reset the mean value
                adc_mean = 0;
                // take the mean value over NO_OF_MEAN
                for (int j = 0; j < NO_OF_MEAN; ++j) {
                    adc_mean += adc1_get_raw((adc1_channel_t) channel) >> MEAN_SHIFT;
                }
                adc_data[i] = adc_mean;
                adc_reading += adc_data[i];
            } else {
                // reset the mean value
                adc_mean = 0;
                // take the mean value over NO_OF_MEAN
                int adc_temp = 0;
                esp_err_t err = ESP_OK;
                for (int j = 0; j < NO_OF_MEAN; ++j) {
                    err = adc2_get_raw((adc1_channel_t) channel,  ADC_WIDTH_BIT_12, &adc_temp);
                    if (err != ESP_OK) {
                        printf("error adc\r\n");
                    }
                    adc_mean += adc_temp >> MEAN_SHIFT;
                }
                adc_data[i] = adc_mean;
                adc_reading += adc_data[i];
            }
        }

        adc_reading /= NO_OF_SAMPLES/NO_OF_MEAN;
        // get the maximum value
        adc_max = 0;
        for (int k = 0; k < NO_OF_SAMPLES/NO_OF_MEAN; ++k) {
            if(adc_data[k] > adc_max) {
                adc_max = adc_data[k];
            }
        }
        //Convert adc_reading to voltage in mV
        uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_max, adc_chars);
        for (int j = 0; j < NO_OF_SAMPLES/NO_OF_MEAN ; ++j) {
            printf(" %d,", adc_data[j]);
        }
        printf("\r\n");
        float current = voltage * 2500 / 1.414;
        printf("Raw: %d\tVoltage: %dmV\tCurrent: %2.6f\n", adc_reading, voltage,current);


        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

