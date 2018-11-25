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
#include <esp_log.h>
#include <home/ESP/test2/esp-idf/components/vfs/include/esp_vfs_dev.h>


#include "util.h"
#include "wifi.h"
#include "sntp_time.h"


#include "key_handling.h"
#include "ubirch_proto_http.h"
#include "ubirch_console.h"

//#include "temp_sensor.h"
#include "temp_temp.h"
#include "esp32-hal-adc.h"
#include "storage.h"


// message response
extern int response;

extern EventGroupHandle_t wifi_event_group;
//extern const int CONNECTED_BIT = BIT0;

#define BLUE_LED GPIO_NUM_2
#define BOOT_BUTTON GPIO_NUM_0
#define ESP_INTR_FLAG_DEFAULT 0


char *TAG = "example-esp32";

void app_main() {

    uint32_t io_num;


    initialize_nvs();


#if CONFIG_STORE_HISTORY
    initialize_filesystem();
#endif

    set_hw_ID();
    check_key_status();


    ESP_LOGI(TAG, "connecting to wifi");
    struct Wifi_login wifi;

//    char *ssid = {"Welt"};
//    char *pwd = {"SpielSpass"};
    if (!load_wifi_login(&wifi)) {

        if (!wifi_join(wifi, 5000)) {
            ESP_LOGI(TAG, "established");
            obtain_time();
            register_keys();
        }
    }


    // set the blue LED pin on the ESP32 DEVKIT V1 board
    gpio_set_direction(BLUE_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(BOOT_BUTTON, GPIO_MODE_INPUT);
//
    int32_t values[2];

    int hall_value = 0;

    while (true) {
        // let the LED blink
        if (response < 1000) {
            gpio_set_level(BLUE_LED, 0);
        } else gpio_set_level(BLUE_LED, 1);

        values[0] = hallRead();
        float f_temperature = temperatureRead();
        values[1] = (int32_t) (f_temperature);

        ESP_LOGI(TAG, "Hall Sensor = %d \r\nTemp. Sensor = %f", values[0], f_temperature);

        create_message(values, 2);

        if (gpio_get_level(BOOT_BUTTON) == 0) {
            ESP_LOGI(TAG, "Starting Console");
            run_console();
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

}



