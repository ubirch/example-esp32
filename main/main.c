
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <nvs_flash.h>

#include "msgpack.h"
#include "ubirch_protocol.h"
#include "ubirch_ed25519.h"


#include "util.h"
#include "http.h"
#include "wifi.h"
#include "sntpTime.h"
#include "keyHandling.h"

char *TAG = "example-esp32";

extern int response;


void app_main(void)
{
    printf("\r\n hello this is wifi example \r\n\n");
    nvs_flash_init();

    getSetUUID();

    http_init_unpacker();
    my_wifi_init();


    char data[32];
    sprintf(data,"hello \r\n");

    obtain_time();

    checkKeyStatus();
//
//    tcp_client_send_msg(data, 0);
    int i = 0;

    //time_t timeX = time();
    esp_timer_get_time();

//    registerKeys();


    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);

    while (true) {
        getTimeUs();
        create_message();
        vTaskDelay(5000 /portTICK_PERIOD_MS);

        if(response < 1000) {
            gpio_set_level(GPIO_NUM_2, 0);
        } else gpio_set_level(GPIO_NUM_2, 1);
    }
}
