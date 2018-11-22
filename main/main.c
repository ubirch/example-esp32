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



#include "util.h"
#include "wifi.h"
#include "sntp_time.h"


#include "key_handling.h"
#include "ubirch_proto_http.h"
#include "ubirch_console.h"

//#include "temp_sensor.h"
#include "temp_temp.h"
#include "esp32-hal-adc.h"


// message response
extern int response;

extern EventGroupHandle_t wifi_event_group;
//extern const int CONNECTED_BIT = BIT0;

#define BLUE_LED GPIO_NUM_2

char *TAG = "example-esp32";


void app_main() {
    initialize_nvs();

#if CONFIG_STORE_HISTORY
    initialize_filesystem();
#endif

    set_hw_ID();
    check_key_status();

//    initSensor();

//    run_console();

    ESP_LOGI(TAG, "connecting to wifi");
    //  char ssid[64], pwd[64];
//    char *ssid = {"Welt"};
//    char *pwd = {"SpielSpass"};
////    if (!load_wifi_login(ssid, pwd)) {
//
//        if (wifi_join(ssid, pwd, 5000)) {
//            ESP_LOGI(TAG, "established");
//        }
////    }
//
//    /* Resume program */
//    printf("\r\n xxx \r\n\n");
////    nvs_flash_init();
//
////    my_wifi_init();
//
//    obtain_time();
////    check_key_status();
//
//    register_keys();
//    // set the blue LED pin on the ESP32 DEVKIT V1 board
//    gpio_set_direction(BLUE_LED, GPIO_MODE_OUTPUT);
//
//    uint32_t values[2];

    int hall_value = 0;

    while (true) {
//        if (dht11_read_val(values)) {
//            vTaskDelay(1000 / portTICK_PERIOD_MS);
//            if (dht11_read_val(values)) {
//                vTaskDelay(1000 / portTICK_PERIOD_MS);
//                dht11_read_val(values);
//            }
//        }
//        create_message(values);
//        // let the LED blink
//        if (response < 1000) {
//            gpio_set_level(BLUE_LED, 0);
//        } else gpio_set_level(BLUE_LED, 1);
//        printf("hahahall = %d",hall_value);

        vTaskDelay(1000 / portTICK_PERIOD_MS);

        hall_value = hallRead();
        ESP_LOGI(TAG, "hahahall = %d", hall_value);
        DHT_read();


//        char* line = linenoise(prompt);
//
//        if (line == NULL) { /* Ignore empty lines */
//            continue;
//        }
//        else {
//            printf("do not touch \r\n");
//        }

    }

}


