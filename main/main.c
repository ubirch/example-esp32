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
#include <../../components/vfs/include/esp_vfs_dev.h>
#include <home/ESP/arduino_lib_test/esp-idf/components/driver/include/driver/adc.h>

//#include "../../components/arduino-esp32/cores/esp32/Arduino.h"
#include "esp32-hal.h"
#include "esp32-hal-adc.h"

#include "util.h"
#include "wifi.h"
#include "sntp_time.h"


#include "key_handling.h"
#include "ubirch_proto_http.h"
#include "ubirch_console.h"

//#include "temp_sensor.h"
#include "temp_temp.h"
//#include "esp32-hal-adc.h"
#include "storage.h"

static SemaphoreHandle_t sem_wifi_initialized = NULL;



// message response
extern int response;

extern EventGroupHandle_t wifi_event_group;
//extern const int CONNECTED_BIT = BIT0;

#define BLUE_LED GPIO_NUM_2
#define BOOT_BUTTON GPIO_NUM_0
#define ESP_INTR_FLAG_DEFAULT 0


char *TAG = "example-esp32";

extern uint8_t temprature_sens_read();

static TaskHandle_t main_task_handle = NULL;
static TaskHandle_t net_config_handle = NULL;

void main_task(void *pvParameters){

    for(;;) {
        int32_t values[2];
        // let the LED blink
        if (response < 1000) {
            gpio_set_level(BLUE_LED, 0);
        } else gpio_set_level(BLUE_LED, 1);

        values[0] = hallRead();
        float f_temperature = temperatureRead();
        values[1] = (int32_t) (f_temperature);

        ESP_LOGI(TAG, "Hall Sensor = %d \r\nTemp. Sensor = %f", values[0], f_temperature);

  //      create_message(values, 2);

        vTaskDelay(30000 / portTICK_PERIOD_MS);
    }
}

void network_config_task(void *pvParameters){

    for(;;){}
//    if( xSemaphoreTake(sem_wifi_initialized, (TickType_t) 10) == pdTRUE){
//        ESP_LOGI("semaphore check", "wifi enabled");
//        obtain_time();
//        register_keys();

//        vTaskDelete(net_config_handle);
//    }
//    else {
//        ESP_LOGW("network configuration", "not performed yet");
//    }

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

    set_hw_ID();
    check_key_status();

    return err;
}



void app_main() {

    init_system();
    vSemaphoreCreateBinary(sem_wifi_initialized);


    ESP_LOGI(TAG, "connecting to wifi");
    struct Wifi_login wifi;

    if (!load_wifi_login(&wifi)) {

        if (wifi_join(wifi, 5000)) {
            ESP_LOGI(TAG, "established");
            xSemaphoreGive(sem_wifi_initialized);
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

    while (true) {
        char c;
        c = fgetc(stdin);
        if(c != 0x03) {  //0x03 = Ctrl + C
            vTaskDelay(500 / portTICK_PERIOD_MS);
            continue;
        }
        // If Ctrl + C was pressed, enter the console and suspend the other tasks until console exits.
        ESP_LOGI(TAG, "Starting Console");
        vTaskSuspend(main_task_handle);
        run_console();
        vTaskResume(main_task_handle);
    }
}



