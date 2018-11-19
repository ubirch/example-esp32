//
// Created by wowa on 15.11.18.
//

#include "temp_temp.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//#include <home/ESP/test2/esp-idf/components/driver/include/driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>


#define MAX_TIME 85
#define DHT11PIN GPIO_NUM_4
#define HIGH 1
#define LOW 0
int dht11_val[5] = {0, 0, 0, 0, 0};


void delay_us(uint32_t time) {
    for (int j = 0; j < time; ++j) {
        for (int i = 0; i < 28; ++i) {
            asm("NOP");
        }
    }
}

bool dht11_read_val(uint32_t *values) {
    uint8_t lststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;
    for (i = 0; i < 5; i++)
        dht11_val[i] = 0;
    gpio_set_direction(DHT11PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11PIN, LOW);
    vTaskDelay(18 / portTICK_PERIOD_MS);
    gpio_set_level(DHT11PIN, HIGH);
    delay_us(34);
    gpio_set_direction(DHT11PIN, GPIO_MODE_INPUT);
    for (i = 0; i < MAX_TIME; i++) {
        counter = 0;
        while (gpio_get_level(DHT11PIN) == lststate) {
            counter++;
            delay_us(1);
            if (counter == 255)
                break;
        }
        lststate = (uint8_t) gpio_get_level(DHT11PIN);
        if (counter == 255)
            break;
        // top 3 transistions are ignored
        if ((i >= 4) && (i % 2 == 0)) {
            dht11_val[j / 8] <<= 1;
            if (counter > 29)
                dht11_val[j / 8] |= 1;
            j++;
        }
    }
    // verify cheksum and print the verified data
    if ((j >= 40) && (dht11_val[4] == ((dht11_val[0] + dht11_val[1] + dht11_val[2] + dht11_val[3]) & 0xFF))) {
        printf("H = %d.%d\nT = %d.%d\n", dht11_val[0], dht11_val[1], dht11_val[2], dht11_val[3]);
        values[0] = (uint32_t) (dht11_val[0] * 10 + dht11_val[1]);
        values[1] = (uint32_t) (dht11_val[2] * 10 + dht11_val[3]);
        return false;
    } else
        printf("Invalid Data!!\n");
    return true;
}


static void tttimer_start(uint32_t time) {
    gpio_set_level(DHT11PIN, LOW);

}

void test_timing(void) {
    gpio_set_direction(DHT11PIN, GPIO_MODE_OUTPUT);

    for (int i = 0; i < 100; ++i) {
//        timer_low_high(i);
        vTaskDelay(1);
    }
}

static void oneshot_timer_callback(void *arg) {
    gpio_set_level(DHT11PIN, HIGH);
}

