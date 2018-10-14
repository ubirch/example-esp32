/*!
 * @file    main.c
 * @brief   main function of the example program
 *
 * @author Waldemar Gruenwald
 * @date   2018-10-10
 *
 * @copyright &copy; 2018 ubirch GmbH (https://ubirch.com)
 *
 * ```
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * ```
 */

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <nvs_flash.h>

#include "msgpack.h"
#include "ubirch_protocol.h"
#include "ubirch_ed25519.h"


#include "util.h"
#include "http.h"
#include "wifi.h"
#include "sntpTime.h"
#include "keyHandling.h"

char *TAG = "example-esp32";

// message response
extern int response;

#define BLUE_LED GPIO_NUM_2


void app_main(void) {
    printf("\r\n hello this is Ubirch protocol on ESP32 wifi example \r\n\n");
    nvs_flash_init();
    getSetUUID();

    http_init_unpacker();
    my_wifi_init();

    obtain_time();
    checkKeyStatus();
    // set the blue LED pin on the ESP32 DEVKIT V1 board
    gpio_set_direction(BLUE_LED, GPIO_MODE_OUTPUT);

    while (true) {
        create_message();
        // let the
        if (response < 1000) {
            gpio_set_level(BLUE_LED, 0);
        } else gpio_set_level(BLUE_LED, 1);

        vTaskDelay(30000 / portTICK_PERIOD_MS);
    }
}
