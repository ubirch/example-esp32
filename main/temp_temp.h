//
// Created by wowa on 15.11.18.
//

#ifndef EXAMPLE_ESP32_TEMP_TEMP_H
#define EXAMPLE_ESP32_TEMP_TEMP_H

#include <stdint.h>
#include <home/ESP/test2/esp-idf/components/esp32/include/esp32/pm.h>
// how many timing transitions we need to keep track of. 2 * number bits + extra
#define MAXTIMINGS 85

bool dht11_read_val(uint32_t *values);

void test_timing(void);

bool DHT_read();
#endif //EXAMPLE_ESP32_TEMP_TEMP_H
