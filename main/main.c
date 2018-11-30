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
#include "storage.h"
//#include <../../components/vfs/include/esp_vfs_dev.h>
//#include <home/ESP/arduino_lib_test/esp-idf/components/driver/include/driver/adc.h>

//#include "../../components/arduino-esp32/cores/esp32/Arduino.h"
#include "esp32-hal.h"
#include "esp32-hal-adc.h"

#include "util.h"
#include "networking.h"
#include "sntp_time.h"


#include "key_handling.h"
#include "ubirch_proto_http.h"
#include "ubirch_console.h"

//#include "temp_sensor.h"
#include "temp_temp.h"
//#include "esp32-hal-adc.h"

// message response
extern int response;

#define BLUE_LED GPIO_NUM_2
#define BOOT_BUTTON GPIO_NUM_0
#define ESP_INTR_FLAG_DEFAULT 0


char *TAG = "example-esp32";

extern uint8_t temprature_sens_read();

static TaskHandle_t main_task_handle = NULL;
static TaskHandle_t net_config_handle = NULL;
static TaskHandle_t console_handle = NULL;

extern EventGroupHandle_t wifi_event_group;


void main_task(void *pvParameters){
    // get the current tick count to run the task periodic
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint32_t task_period_ms = 3000; //!< period for the task in ms, default value = 30 sec.
    if (pvParameters != NULL){
        task_period_ms = *(uint32_t*)(pvParameters);
    }

    EventBits_t event_bits;

    for(;;) {
        event_bits = xEventGroupWaitBits(wifi_event_group,
                                         READY_BIT,
                                         false,
                                         false,
                                         portMAX_DELAY);
        //
        if (event_bits & READY_BIT) {
            int32_t values[2];
            // let the LED blink
            if (response < 1000) {
                gpio_set_level(BLUE_LED, 0);
            } else gpio_set_level(BLUE_LED, 1);

            values[0] = hallRead();
            float f_temperature = temperatureRead();
            values[1] = (int32_t)(f_temperature);
            ESP_LOGI(TAG, "Hall Sensor = %d \r\nTemp. Sensor = %f", values[0], f_temperature);
            create_message(values, 2);
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(task_period_ms));
        }
    }
}

void network_config_task(void *pvParameters){
    ESP_LOGI("network config", "started");
    EventBits_t event_bits;

    for(;;) {
        event_bits = xEventGroupWaitBits(wifi_event_group,
                                         (CONNECTED_BIT | CONFIGURED_BIT | READY_BIT),
                                         false,
                                         false,
                                         portMAX_DELAY);
        //
        if (event_bits == CONNECTED_BIT) {
            ESP_LOGI("event group bit check", "wifi enabled");
            obtain_time();
            register_keys();
            //
            xEventGroupSetBits(wifi_event_group, READY_BIT);
        }
        else {
            vTaskDelay(portMAX_DELAY);
        }
    }
}

void enter_console(void *pvParameter){
    char c;
    for (;;) {
        c = fgetc(stdin);
        if(c == 0x03) {  //0x03 = Ctrl + C
            // If Ctrl + C was pressed, enter the console and suspend the other tasks until console exits.
            ESP_LOGI(TAG, "Starting Console");
            vTaskSuspend(main_task_handle);
            vTaskSuspend(net_config_handle);
            run_console();
            vTaskResume(net_config_handle);
            vTaskResume(main_task_handle);
        }
    }
}

/**
 * Initialize the basic system components
 * @return ESP_OK or error code.
 */
esp_err_t init_system() {

    esp_err_t err = ESP_OK;

    // set the blue LED pin on the ESP32 DEVKIT V1 board
    gpio_set_direction(BLUE_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(BOOT_BUTTON, GPIO_MODE_INPUT);

    init_nvs();

#if CONFIG_STORE_HISTORY
    initialize_filesystem();
#endif

    init_console();
    initialise_wifi();


    set_hw_ID();
    check_key_status();

    return err;
}



void app_main() {

    esp_err_t err;
    init_system();

    ESP_LOGI(TAG, "connecting to wifi");
    struct Wifi_login wifi;

//    memory_error_check(kv_store("wifi_data", "wifi_ssid", "FTWK Fritzbox", 14));
//    memory_error_check(kv_store("wifi_data", "wifi_pwd", "HELLOFTWK", 10));

    err = kv_load("wifi_data", "wifi_ssid", &wifi.ssid, &wifi.ssid_length);
    ESP_LOGD(TAG, "%s", wifi.ssid);
    if(err == ESP_OK) {
        kv_load("wifi_data", "wifi_pwd", &wifi.pwd, &wifi.pwd_length);
        ESP_LOGD(TAG, "%s", wifi.pwd);
        if (wifi_join(wifi, 5000)) {
            ESP_LOGI(TAG, "established");
        }
        else { // no connection
            ESP_LOGW(TAG, "no valid Wifi");
        }
    }
    else {  // no WiFi connection
        ESP_LOGW(TAG, "no Wifi login data");
    }

    xTaskCreate(&main_task, "hello_task", 8192, NULL, 5, &main_task_handle);
    xTaskCreate(&network_config_task, "network_config", 4096, NULL, 5, &net_config_handle);
    xTaskCreate(&enter_console, "enter_console", 4096, NULL, 1, &console_handle );

    while (true) {
    }
}



