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
#include <esp_log.h>
#include <networking.h>
#include <sntp_time.h>
#include <ubirch_console.h>
#include <nvs_flash.h>
#include <ubirch_ota_task.h>
#include <ubirch_ota.h>

#include "storage.h"
#include "key_handling.h"
#include "util.h"
#include "sensor.h"
#include "oled.h"

char *TAG = "example-esp32";

static TaskHandle_t fw_update_task_handle = NULL;
static TaskHandle_t main_task_handle = NULL;
static TaskHandle_t net_config_handle = NULL;
static TaskHandle_t console_handle = NULL;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-noreturn"

static void main_task(void *pvParameters) {
    bool keys_registered = false;
    EventBits_t event_bits;
    for (;;) {
        event_bits = xEventGroupWaitBits(network_event_group, NETWORK_STA_READY | NETWORK_ETH_READY,
                                         false, false, portMAX_DELAY);
        //
        if (event_bits & (NETWORK_STA_READY | NETWORK_ETH_READY)) {
            if (!keys_registered) {
                register_keys();
                keys_registered = true;
            }
            sensor_loop();
            oled_show();
        }
    }
}


static void update_time_task(void __unused *pvParameter) {
    EventBits_t event_bits;

    for (;;) {
        event_bits = xEventGroupWaitBits(network_event_group, (NETWORK_ETH_READY | NETWORK_STA_READY),
                                         false, false, portMAX_DELAY);
        if (event_bits & (NETWORK_ETH_READY | NETWORK_STA_READY)) {
            sntp_update();
        }
        vTaskDelay(18000000);
    }
}

static void enter_console(void *pvParameter) {
    char c;
    for (;;) {
        c = (char) fgetc(stdin);
        if (c == 0x03) {  //0x03 = Ctrl + C
            // If Ctrl + C was pressed, enter the console and suspend the other tasks until console exits.
            vTaskSuspend(main_task_handle);
            vTaskSuspend(net_config_handle);
            run_console();
            vTaskResume(net_config_handle);
            vTaskResume(main_task_handle);
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
    initialise_wifi();

    set_hw_ID();
    check_key_status();

    sensor_setup();

    return err;
}


void app_main() {

    esp_err_t err;
    init_system();

    ESP_LOGI(TAG, "connecting to wifi");
    struct Wifi_login wifi;

    err = kv_load("wifi_data", "wifi_ssid", (void **) &wifi.ssid, &wifi.ssid_length);
    if (err == ESP_OK) {
        ESP_LOGD(TAG, "SSID: %.*s", wifi.ssid_length, wifi.ssid);
        kv_load("wifi_data", "wifi_pwd", (void **) &wifi.pwd, &wifi.pwd_length);
        ESP_LOGD(TAG, "PASS: %.*s", wifi.pwd_length, wifi.pwd);
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
    // force firmware update check after restart
    ubirch_firmware_update();

    xTaskCreate(&main_task, "main", 8192, NULL, 5, &main_task_handle);
    xTaskCreate(&update_time_task, "sntp", 4096, NULL, 5, &net_config_handle);
    xTaskCreate(&enter_console, "console", 4096, NULL, 5, &console_handle);
    xTaskCreate(&ubirch_ota_task, "fw_update", 4096, NULL, 5, &fw_update_task_handle);

    vTaskSuspend(NULL);
}



