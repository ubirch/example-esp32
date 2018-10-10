
#include <tcpip_adapter.h>
#include <esp_event_loop.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <string.h>

#include <freertos/event_groups.h>

#include "settings.h"
#include "wifi.h"

static const char *TAG = "WIFI";
/* FreeRTOS event group to signal when we are connected & ready to make a request */
EventGroupHandle_t wifi_event_group;

bool connected_with_IP = false;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;


static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    printf("EVENT: ");
    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            printf("wifi started \r\n");
            ESP_ERROR_CHECK( esp_wifi_connect() );
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            printf("wifi is connected now \r\n");
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            printf("wifi got ip:" IPSTR "\r\n", IP2STR(&event->event_info.got_ip.ip_info.ip));
            connected_with_IP = true;
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            printf("wifi disconnected\r\n");
            break;
        default:
            printf("unknown/not implemented \r\n");
            break;
    }
    return ESP_OK;
}

/*
 * Initialize the wifi module
 */
void my_wifi_init(void){
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t sta_config = {
            .sta = {
                    .ssid = EXAMPLE_WIFI_SSID,
                    .password = EXAMPLE_WIFI_PASS,
                    .bssid_set = false
            }
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", sta_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_LOGI(TAG, "wifi_init_sta finished.");
}