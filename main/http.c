/* ESP HTTP Client Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include <string.h>
#include <stdlib.h>
#include <msgpack.h>
#include <ubirch_protocol.h>
#include <ubirch_ed25519.h>
#include <msgpack/object.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
//#include "app_wifi.h"

#include "esp_http_client.h"
#include "http.h"
#include "util.h"
#include "keyHandling.h"

#define MAX_HTTP_RECV_BUFFER 512
static const char *TAG = "HTTP_CLIENT";


size_t rcv_buffer_size = 100;
msgpack_unpacker *unpacker = NULL;

void http_init(){
    unpacker = msgpack_unpacker_new(rcv_buffer_size);
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
//    static char * p_buffer = NULL;
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            if(!strcmp("Content-Length",(const char*)(evt->header_key))){
                ESP_LOGD(TAG, "MESSAGE LENGTH %s", evt->header_value);
            }
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // Write out data
                printf("%.*s", evt->data_len, (char*)evt->data);
                print_message(evt->data, evt->data_len);
                if (msgpack_unpacker_buffer_capacity(unpacker) < evt->data_len)
                {
                    msgpack_unpacker_reserve_buffer(unpacker, evt->data_len);
                }
                memcpy(msgpack_unpacker_buffer(unpacker), evt->data, evt->data_len);
                msgpack_unpacker_buffer_consumed(unpacker, evt->data_len);
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}

void http_post_task(const char *url, const char *data, const size_t length) {
    ESP_LOGI(TAG, "post");
    esp_http_client_config_t config = {
            .url = url,
            .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // POST
    esp_http_client_set_url(client, url);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type","application/octet-stream");
    esp_http_client_set_post_field(client, data, (int)(length));
    esp_err_t err = esp_http_client_perform(client);
    bool success = false;
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
        success = (esp_http_client_get_status_code(client) >= 200 && esp_http_client_get_status_code(client) <=299);
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    if(success) {
        check_response();
    }
}

void check_response() {
    ESP_LOGI(TAG, "check");

    if(!ubirch_protocol_verify(unpacker,esp32_ed25519_verify)){
        ESP_LOGI(TAG,"check successful");
        parse_payload(unpacker);
    } else {
        ESP_LOGW(TAG, "verification failed");
    }
}

void parse_payload(msgpack_unpacker *unpacker) {
    ESP_LOGI(TAG, "parsing payload");

    msgpack_unpacked result;
    msgpack_unpacked_init(&result);

    if (msgpack_unpacker_next(unpacker, &result) && result.data.type == MSGPACK_OBJECT_ARRAY) {
        msgpack_object *envelope = result.data.via.array.ptr;

        int p_version = 0;

        // envelope version
        if (envelope->type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            p_version = (int) envelope->via.u64;
            ESP_LOGI(TAG, "VERSION: %d (variant %d)\r\n", p_version >> 4, p_version & 0xf);
        }

        // backend UUID
        if ((++envelope)->type == MSGPACK_OBJECT_RAW) {
            ESP_LOGI(TAG, "UUID: ");
            print_message(envelope->via.raw.ptr, envelope->via.raw.size);
        }

        // only continue if the envelope version and variant match
        if (p_version == proto_chained) {
            // previous message signature (from our request message)
            unsigned char prev_signature[UBIRCH_PROTOCOL_SIGN_SIZE];
            if(loadSignature(prev_signature)){
                ESP_LOGW(TAG,"error loading signature");
            }
            bool prevSignatureMatches = false;
            if ((++envelope)->type == MSGPACK_OBJECT_RAW) {
                ESP_LOGI(TAG, "PREV: ");
                print_message(envelope->via.raw.ptr, envelope->via.raw.size);
                if (envelope->via.raw.size == crypto_sign_BYTES) {
                    prevSignatureMatches = !memcmp(prev_signature, envelope->via.raw.ptr, UBIRCH_PROTOCOL_SIGN_SIZE);
                }
            }

            if (prevSignatureMatches && (++envelope)->type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
                ESP_LOGI(TAG, "TYPE: %d\r\n", (unsigned int) envelope->via.u64);
                switch ((unsigned int) envelope->via.u64) {
                    case MSGPACK_MSG_REPLY:
                        parseMeasurementReply(++envelope);
                        break;
                    case UBIRCH_PROTOCOL_TYPE_HSK:
                        //parseHandshakeReply(++envelope, state, config);
                        break;
                    default:
                        ESP_LOGI(TAG, "unknown packet data type");
                        break;
                }
            } else {
                ESP_LOGI(TAG, "prev signature mismatch or message type wrong!");
            }
        } else {
            ESP_LOGI(TAG, "protocol version mismatch: %d != %d", p_version, proto_chained);
        }
    } else {
        ESP_LOGI(TAG, "empty message not accepted");
    }
    ESP_LOGI(TAG, "destroy result");
    msgpack_unpacked_destroy(&result);
}

int response = 0;
/**
 * Parse a measurement packet reply.
 * @param envelope the packet envelope at the point where the payload starts
 * @param state the current state of the trackle
 * @param config the current trackle configuation
 */
void parseMeasurementReply(msgpack_object *envelope) {
    ESP_LOGI(TAG, "measurement reply");
    if (envelope->type == MSGPACK_OBJECT_MAP) {
        msgpack_object_kv *map = envelope->via.map.ptr;
        for (uint32_t entries = 0; entries < envelope->via.map.size; map++, entries++) {
            if (match(map, "i", MSGPACK_OBJECT_POSITIVE_INTEGER)) {
                // read new interval setting
                unsigned int value = (unsigned int)(map->val.via.u64);
                ESP_LOGI(TAG, "i = %d ", value);
                response = value;
            }
            else ESP_LOGI(TAG, "unknown MSGPACK object in MAP");
        }
    }
    else ESP_LOGI(TAG, "unknown MSGPACK object");
}

/**
 * Helper function, checking a specific key in a msgpack map.
 * @param map the map
 * @param key the key we look for
 * @param type the type of the value we look for
 * @return true if the key and type match
 */
bool match(const msgpack_object_kv *map, const char *key, const int type) {
    ESP_LOGI(TAG, "match");
    const size_t keyLength = strlen(key);
    return map->key.type == MSGPACK_OBJECT_RAW &&
           map->key.via.raw.size == keyLength &&
           (type == -1 || map->val.type == type) &&
           !memcmp(key, map->key.via.raw.ptr, keyLength);
}


