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
#include <time.h>
#include <PN532.h>

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
        event_bits = xEventGroupWaitBits(network_event_group, NETWORK_STA_READY | NETWORK_ETH_READY,
                                         false, false, portMAX_DELAY);
        //
        if (event_bits & (NETWORK_STA_READY | NETWORK_ETH_READY)) {
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
        event_bits = xEventGroupWaitBits(network_event_group, (NETWORK_ETH_READY | NETWORK_STA_READY),
                                         false, false, portMAX_DELAY);
        if (event_bits & (NETWORK_ETH_READY | NETWORK_STA_READY)) {
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

//#define SDA_PIN 21
//#define SCL_PIN 22
//
//
//static char tag[] = "i2cscanner";
//
//void task_i2cscanner(void) {
//    ESP_LOGD(tag, ">> i2cScanner");
//    i2c_config_t conf;
//    conf.mode = I2C_MODE_MASTER;
//    conf.sda_io_num = SDA_PIN;
//    conf.scl_io_num = SCL_PIN;
//    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
//    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
//    conf.master.clk_speed = 400000;
//    i2c_param_config(I2C_NUM_0, &conf);
//
//    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
//
//    int i;
//    esp_err_t espRc;
//    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
//    printf("00:         ");
//    for (i = 0; i < 0xff; i++) {
//        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
//        i2c_master_start(cmd);
//        i2c_master_write_byte(cmd, (i) | I2C_MASTER_WRITE, 1 /* expect ack */);
//        i2c_master_stop(cmd);
//
//        espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10 / portTICK_PERIOD_MS);
//        if (i % 16 == 0) {
//            printf("\n%.2x:", i);
//        }
//        if (espRc == 0) {
//            printf(" %.2x", i);
//        } else {
//            printf(" --");
//        }
//        // ESP_LOGD(tag, "i=%d, rc=%d (0x%x)", i, espRc, espRc);
//        i2c_cmd_link_delete(cmd);
//    }
//    printf("\n");
//    vTaskDelete(NULL);
//}
//

/**
 * Initialize the basic system components
 * @return ESP_OK or error code.
 */
static esp_err_t init_system() {

    esp_err_t err;

    uint8_t uid[7];
    uint8_t uidLength;

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


    if (init_PN532_I2C(21, 22, 12, 19, 0)) {
        ESP_LOGI(TAG, "NFC init complete");
        // send dummy command to wake board up
        getPN532FirmwareVersion();

        setPassiveActivationRetries(0xFF);

        // configure board to read RFID tags
        SAMConfig();
        if (readPassiveTargetID(0, uid, &uidLength, 30000)) {
            ESP_LOGI(TAG, "UID = %02x %02x %02x %02x %02x %02x %02x", uid[0], uid[1], uid[2], uid[3], uid[4], uid[5],
                     uid[6]);
        } else {
            ESP_LOGI(TAG, " FAILED TO READ");
        }
    } else {
        ESP_LOGW(TAG, "NFC init FAILED");
    }


//    uint8_t nfc_buffer[4];
//    for (int i =  0; i < 0xff ; ++i) {
//        mifareultralight_ReadPage(i, nfc_buffer);
//        ESP_LOGI("I", "%d", i);
//        esp_log_buffer_hex("nfc read", nfc_buffer, 4);
//    }

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

    // create the system tasks to be executed
    xTaskCreate(&enter_console_task, "console", 4096, NULL, 7, &console_handle);
    xTaskCreate(&update_time_task, "sntp", 4096, NULL, 4, &net_config_handle);
    xTaskCreate(&ubirch_ota_task, "fw_update", 4096, NULL, 5, &fw_update_task_handle);
    xTaskCreate(&main_task, "main", 8192, NULL, 6, &main_task_handle);

    ESP_LOGI(TAG, "all tasks created");

    while (1) vTaskSuspend(NULL);
}

#pragma GCC diagnostic pop



