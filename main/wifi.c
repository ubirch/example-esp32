/*!
 * @file    wifi.c
 * @brief   Wifi initialization and connection functions
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
#include <string.h>
#include <tcpip_adapter.h>
#include <esp_event_loop.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <freertos/event_groups.h>

#include "settings.h"
#include "wifi.h"

static const char *TAG = "WIFI";
/* FreeRTOS event group to signal when we are connected & ready to make a request */
EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;


static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            ESP_LOGI(TAG, "wifi started");
            ESP_ERROR_CHECK( esp_wifi_connect() );
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "wifi is connected now");
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            ESP_LOGI(TAG, "wifi got ip:" IPSTR " ", IP2STR(&event->event_info.got_ip.ip_info.ip));
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            ESP_LOGI(TAG, "wifi disconnected");
            break;
        default:
            ESP_LOGI(TAG, "unknown/not implemented");
            break;
    }
    return ESP_OK;
}


static void initialise_wifi(void) {
    esp_log_level_set("wifi", ESP_LOG_WARN);
    static bool initialized = false;
    if (initialized) {
        return;
    }
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
    ESP_ERROR_CHECK(esp_wifi_start());
    initialized = true;
}

bool wifi_join(const char *ssid, const char *pass, int timeout_ms) {
    initialise_wifi();
    wifi_config_t wifi_config = {0};
    strncpy((char *) wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    if (pass) {
        strncpy((char *) wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
    }

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());

    int bits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                                   1, 1, timeout_ms / portTICK_PERIOD_MS);

    return (bits & CONNECTED_BIT) != 0;
}
