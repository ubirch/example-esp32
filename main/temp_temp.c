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
#include <esp32-hal-gpio.h>


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

uint8_t data[6];
uint8_t _pin = DHT11PIN, _type, _count = 60;

bool read(void);

unsigned long _lastreadtime;
bool firstreading;


bool DHT_read(void) {
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;
    unsigned long currenttime;

    // pull the pin high and wait 250 milliseconds
    digitalWrite(_pin, HIGH);
    delay(250);

    currenttime = millis();
    if (currenttime < _lastreadtime) {
        // ie there was a rollover
        _lastreadtime = 0;
    }
    if (!firstreading && ((currenttime - _lastreadtime) < 2000)) {
        return true; // return last correct measurement
        //delay(2000 - (currenttime - _lastreadtime));
    }
    firstreading = false;
    /*
      Serial.print("Currtime: "); Serial.print(currenttime);
      Serial.print(" Lasttime: "); Serial.print(_lastreadtime);
    */
    _lastreadtime = millis();

    data[0] = data[1] = data[2] = data[3] = data[4] = 0;

    // now pull it low for ~20 milliseconds
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    delay(20);
    //cli();
    digitalWrite(_pin, HIGH);
    delayMicroseconds(40);
    pinMode(_pin, INPUT);

    // read in timings
    for (i = 0; i < MAXTIMINGS; i++) {
        counter = 0;
        while (digitalRead(_pin) == laststate) {
            counter++;
            delayMicroseconds(1);
            if (counter == 255) {
                break;
            }
        }
        laststate = digitalRead(_pin);

        if (counter == 255) break;

        // ignore first 3 transitions
        if ((i >= 4) && (i % 2 == 0)) {
            // shove each bit into the storage bytes
            data[j / 8] <<= 1;
            if (counter > _count)
                data[j / 8] |= 1;
            j++;
        }

    }

    //sei();

    /*
    Serial.println(j, DEC);
    Serial.print(data[0], HEX); Serial.print(", ");
    Serial.print(data[1], HEX); Serial.print(", ");
    Serial.print(data[2], HEX); Serial.print(", ");
    Serial.print(data[3], HEX); Serial.print(", ");
    Serial.print(data[4], HEX); Serial.print(" =? ");
    Serial.println(data[0] + data[1] + data[2] + data[3], HEX);
    */

//    if ((j >= 40) && (dht11_val[4] == ((dht11_val[0] + dht11_val[1] + dht11_val[2] + dht11_val[3]) & 0xFF))) {
//        printf("H = %d.%d\nT = %d.%d\n", dht11_val[0], dht11_val[1], dht11_val[2], dht11_val[3]);
//        values[0] = (uint32_t) (dht11_val[0] * 10 + dht11_val[1]);
//        values[1] = (uint32_t) (dht11_val[2] * 10 + dht11_val[3]);
//        return false;
//    } else
//        printf("Invalid Data!!\n");
//    return true;

    // check we read 40 bits and that the checksum matches
    if ((j >= 40) &&
        (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
        printf("H = %d.%d\nT = %d.%d\n", data[0], data[1], data[2], data[3]);
        return true;
    }

    printf("Invalid Data!!\n");

    return false;

}