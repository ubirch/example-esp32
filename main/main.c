
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_task_wdt.h>
#include <nvs_flash.h>
#include <time.h>
#include <esp_log.h>


#include "msgpack.h"
#include "ubirch_protocol.h"
#include "ubirch_ed25519.h"


#include "util.h"
#include "networkUtils.h"
#include "http.h"
#include "wifi.h"
#include "sntpTime.h"
#include "keyHandling.h"

extern unsigned char UUID[16];

extern EventGroupHandle_t wifi_event_group;
extern bool connected_with_IP;
extern const int CONNECTED_BIT;

char *TAG = "example";

/*
 * create a msgpack message
 */
void create_message(void){
// create buffer, writer, ubirch protocol context and packer
    msgpack_sbuffer *sbuf = msgpack_sbuffer_new();
    ubirch_protocol *proto = ubirch_protocol_new(proto_chained, 83,
                                                 sbuf, msgpack_sbuffer_write, esp32_ed25519_sign, UUID);
    msgpack_packer *pk = msgpack_packer_new(proto, ubirch_protocol_write);
// load the old signature and copy it to the protocol
    unsigned char old_signature[UBIRCH_PROTOCOL_SIGN_SIZE] = {};
    if(loadSignature(old_signature)){
        ESP_LOGW(TAG,"error loading the old signature");
    }
    memcpy(proto->signature, old_signature, UBIRCH_PROTOCOL_SIGN_SIZE);
// start the protocol
    ubirch_protocol_start(proto, pk);

// create map("data": array[ timstamp, value ])
    msgpack_pack_map(pk, 1);
    char *dataPayload = "data";
    msgpack_pack_raw(pk, strlen(dataPayload));
    msgpack_pack_raw_body(pk, dataPayload, strlen(dataPayload));
// create array[ timestamp, value ])
    msgpack_pack_array(pk, 2);
    uint64_t ts = getTimeUs();
    msgpack_pack_uint64(pk, ts);
    uint32_t fake_temp =(esp_random() & 0x0F);
    msgpack_pack_int32(pk, (int32_t)(fake_temp));
// finish the protocol
    ubirch_protocol_finish(proto, pk);
    if(storeSignature(proto->signature, UBIRCH_PROTOCOL_SIGN_SIZE)){
        ESP_LOGW(TAG,"error storing the signature");
    }
// send the message
    print_message((const char *)(sbuf->data), (size_t)(sbuf->size));
    http_post_task(UHTTP_URL, (const char *) (sbuf->data), sbuf->size);

    ESP_LOGI(TAG,"cleaning up");
// clear buffer for next message
    msgpack_sbuffer_clear(sbuf);
// free packer and protocol
    msgpack_packer_free(pk);
    ubirch_protocol_free(proto);
}



extern int response;


void app_main(void)
{
    printf("\r\n hello this is wifi example \r\n\n");
    nvs_flash_init();

    getSetUUID();

    http_init();
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

//1539174937	5bbdf219	39230472
//1539174968	5bbdf238	70334471