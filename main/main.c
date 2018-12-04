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
#include <esp32-hal.h>
#include <networking.h>
#include <sntp_time.h>
#include <ubirch_console.h>
#include <msgpack.h>
#include <response.h>
#include <message.h>
#include <ubirch_api.h>

#include "storage.h"
#include "key_handling.h"
#include "util.h"

#define BLUE_LED GPIO_NUM_2
#define BOOT_BUTTON GPIO_NUM_0
#define ESP_INTR_FLAG_DEFAULT 0


char *TAG = "example-esp32";

extern uint8_t temprature_sens_read();

static TaskHandle_t main_task_handle = NULL;
static TaskHandle_t net_config_handle = NULL;
static TaskHandle_t console_handle = NULL;

extern EventGroupHandle_t wifi_event_group;

extern unsigned char UUID[16];
TickType_t interval = CONFIG_UBIRCH_DEFAULT_INTERVAL;

static void response_handler(const msgpack_object_kv *entry) {
    if (match(entry, "i", MSGPACK_OBJECT_POSITIVE_INTEGER)) {
        interval = (unsigned int) (entry->val.via.u64);
    } else {
        ESP_LOGW(TAG, "unknown configuration received: %.*s", entry->key.via.raw.size, entry->key.via.raw.ptr);
    }
}

static esp_err_t send_message(float temperature, float humidity) {
    msgpack_sbuffer *sbuf = msgpack_sbuffer_new(); //!< send buffer
    msgpack_unpacker *unpacker = msgpack_unpacker_new(128); //!< receive unpacker

    int32_t values[2] = {(int32_t) (temperature * 100), (int32_t) (humidity * 100)};
    ubirch_message(sbuf, UUID, values, sizeof(values) / sizeof(values[0]));

    ubirch_send(CONFIG_UBIRCH_BACKEND_DATA_URL, sbuf->data, sbuf->size, unpacker);
    ubirch_parse_response(unpacker, response_handler);

    msgpack_unpacker_free(unpacker);
    msgpack_sbuffer_free(sbuf);

    return ESP_OK;
}

void main_task(void *pvParameters) {
    // get the current tick count to run the task periodic
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint32_t task_period_ms = 3000; //!< period for the task in ms, default value = 30 sec.
    if (pvParameters != NULL) {
        task_period_ms = *(uint32_t *) (pvParameters);
    }

    EventBits_t event_bits;

    for (;;) {
        event_bits = xEventGroupWaitBits(wifi_event_group, READY_BIT, false, false, portMAX_DELAY);
        //
        if (event_bits & READY_BIT) {
            // let the LED blink
            if (interval < 1000) gpio_set_level(BLUE_LED, 0); else gpio_set_level(BLUE_LED, 1);

            int f_hall = hallRead();
            float f_temperature = temperatureRead();
            ESP_LOGI(TAG, "Hall Sensor = %d ", f_hall);
            ESP_LOGI(TAG, "Temp Sensor = %f", f_temperature);

            send_message(f_hall, f_temperature);

            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(task_period_ms));
        }
    }
}

void network_config_task(void *pvParameters) {
    ESP_LOGI("network config", "started");
    EventBits_t event_bits;

    for (;;) {
        event_bits = xEventGroupWaitBits(wifi_event_group,
                                         (CONNECTED_BIT | CONFIGURED_BIT | READY_BIT),
                                         false,
                                         false,
                                         portMAX_DELAY);
        // only update the sntp time and register the key, when a network connection
        // has currently been established.
        if (event_bits == CONNECTED_BIT) {
            ESP_LOGI("event group bit check", "wifi enabled");
            sntp_update();
            register_keys();
            //
            xEventGroupSetBits(wifi_event_group, READY_BIT);
        } else {
            vTaskDelay(portMAX_DELAY);
        }
    }
}

void enter_console(void *pvParameter) {
    char c;
    for (;;) {
        c = fgetc(stdin);
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

    xTaskCreate(&main_task, "hello_task", 8192, NULL, 5, &main_task_handle);
    xTaskCreate(&network_config_task, "network_config", 4096, NULL, 5, &net_config_handle);
    xTaskCreate(&enter_console, "enter_console", 4096, NULL, 1, &console_handle);

    vTaskSuspend(NULL);
}



