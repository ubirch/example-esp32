//
// Created by wowa on 15.11.18.
//

#ifndef EXAMPLE_ESP32_TEMP_TEMP_H
#define EXAMPLE_ESP32_TEMP_TEMP_H

#include <stdint.h>
#include <home/ESP/test2/esp-idf/components/esp32/include/esp32/pm.h>

bool dht11_read_val(uint32_t *values);

void test_timing(void);

#endif //EXAMPLE_ESP32_TEMP_TEMP_H
