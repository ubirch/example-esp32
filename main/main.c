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

//#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <networking.h>
#include <sntp_time.h>
#include <ubirch_console.h>
#include <nvs_flash.h>
#include <ubirch_ota_task.h>
#include <ubirch_ota.h>
#include <time.h>

#include "storage.h"
#include "key_handling.h"
#include "util.h"
#include "sensor.h"

char *TAG = "example-esp32";

// define task handles for every task
static TaskHandle_t fw_update_task_handle = NULL;
static TaskHandle_t main_task_handle = NULL;
static TaskHandle_t net_config_handle = NULL;
static TaskHandle_t console_handle = NULL;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-noreturn"

/*!
 * Main task performs the main functionality of the application,
 * when the network is set up.
 *
 * @param pvParameters are currently not used, but part of the task declaration.
 */
static void main_task(void *pvParameters) {
    bool keys_registered = false;
    bool force_fw_update = false;
    EventBits_t event_bits;
	for (;;) {
	    event_bits = xEventGroupWaitBits(network_event_group, WIFI_CONNECTED_BIT | NETWORK_ETH_READY,
                                         false, false, portMAX_DELAY);
        //
        if (event_bits & (WIFI_CONNECTED_BIT | NETWORK_ETH_READY)) {
            // after the device is ready, first try a firmwate update
            if (!force_fw_update) {
                ubirch_firmware_update();
                force_fw_update = true;
            }
            if (!keys_registered) {
                // check that we have current time before trying to generate/register keys
                time_t now = 0;
                struct tm timeinfo = {0};
                time(&now);
                localtime_r(&now, &timeinfo);
                if (timeinfo.tm_year >= (2017 - 1900)) {
                    // force time update before generating keys
                    check_key_status();
                    register_keys();
                    keys_registered = true;
                }
            }
            //
            // APPLY  YOUR APPLICATION SPECIFIC CODE HERE
            //
            sensor_loop();
	        ESP_LOGI("HEAP","%d",esp_get_minimum_free_heap_size());
        }
    }
}

/*!
 * Update the system time every 6 hours.
 *
 * @param pvParameters are currently not used, but part of the task declaration.
 */
static void update_time_task(void __unused *pvParameter) {
    EventBits_t event_bits;
	for (;;) {
        event_bits = xEventGroupWaitBits(network_event_group, (NETWORK_ETH_READY | WIFI_CONNECTED_BIT),
                                         false, false, portMAX_DELAY);
        if (event_bits & (NETWORK_ETH_READY | WIFI_CONNECTED_BIT)) {
            sntp_update();
        }
        vTaskDelay(21600000);  // delay this task for the next 6 hours
    }
}

/*!
 *
 * @param pvParameters are currently not used, but part of the task declaration.
 */
static void enter_console_task(void *pvParameter) {
    char c;
	for (;;) {
        c = (char) fgetc(stdin);
        printf("%02x\r", c);
        if ((c == 0x03) || (c == 0x15)) {  //0x03 = Ctrl + C      0x15 = Ctrl + U
            // If Ctrl + C was pressed, enter the console and suspend the other tasks until console exits.
            if (main_task_handle) vTaskSuspend(main_task_handle);
            if (fw_update_task_handle) vTaskSuspend(fw_update_task_handle);
            if (net_config_handle) vTaskSuspend(net_config_handle);
            run_console();
            if (fw_update_task_handle) vTaskResume(net_config_handle);
            if (net_config_handle) vTaskResume(fw_update_task_handle);
            if (main_task_handle) vTaskResume(main_task_handle);
        }
    }
}

#pragma GCC diagnostic pop

/**
 * Initialize the basic system components
 * @return ESP_OK or error code.
 */
static esp_err_t init_system() {

    esp_err_t err;
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // OTA app partition table has a smaller NVS partition size than the non-OTA
        // partition table. This size mismatch may cause NVS initialization to fail.
        // If this happens, we erase NVS partition and initialize NVS again.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    init_nvs();

#if CONFIG_STORE_HISTORY
    initialize_filesystem();
#endif

    init_console();
    init_wifi();

    set_hw_ID();

    sensor_setup();

    return err;
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
/*!
 * Application main function.
 * This function represents the main() functionality of other systems.
 */
void app_main() {

    esp_err_t err;

    // initialize the system
    init_system();

    ESP_LOGI(TAG, "connecting to wifi");
    struct Wifi_login wifi;
    wifi.ssid = NULL;
    wifi.ssid_length = 0;
    wifi.pwd = NULL;
    wifi.pwd_length = 0;

    err = kv_load("wifi_data", "wifi_ssid", (void **) &wifi.ssid, &wifi.ssid_length);
    if (err == ESP_OK) {
        ESP_LOGD(TAG, "SSID: %.*s", wifi.ssid_length, wifi.ssid);
        kv_load("wifi_data", "wifi_pwd", (void **) &wifi.pwd, &wifi.pwd_length);
        //ESP_LOGD(TAG, "PASS: %.*s", wifi.pwd_length, wifi.pwd);
        if (wifi_join(wifi, 5000) == ESP_OK) {
            ESP_LOGI(TAG, "established");
        } else { // no connection possible
            ESP_LOGW(TAG, "no valid Wifi");
        }
        free(wifi.ssid);
        free(wifi.pwd);
    } else {  // no WiFi connection
        ESP_LOGW(TAG, "no Wifi login data");
    }

    // create the system tasks to be executed
    xTaskCreate(&enter_console_task, "console", 4096, NULL, 7, &console_handle);
    xTaskCreate(&update_time_task, "sntp", 4096, NULL, 4, &net_config_handle);
    xTaskCreate(&ubirch_ota_task, "fw_update", 4096, NULL, 5, &fw_update_task_handle);
    xTaskCreate(&main_task, "main", 8192, NULL, 6, &main_task_handle);

    ESP_LOGI(TAG, "all tasks created");

    while (1) vTaskSuspend(NULL);
}

#pragma GCC diagnostic pop



