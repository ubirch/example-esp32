/*!
 * @file    http.c
 * @brief   http functions to send ubirch protocol messages via http post
 *          and to evaluate the response.
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
 *
 * The code was taken from:
 *
 * ESP HTTP Client Example
 *
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 *
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 */

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include <string.h>
#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include "esp_log.h"


#include "msgpack.h"
#include "ubirch_protocol.h"
#include "ubirch_ed25519.h"


#include "esp_http_client.h"
#include "util.h"
#include "key_handling.h"
#include "settings.h"
#include "sntp_time.h"
#include "ubirch_proto_http.h"

#define MAX_HTTP_RECV_BUFFER 512

extern unsigned char UUID[16];

static const char *TAG = "HTTP_CLIENT";

size_t rcv_buffer_size = 100;
msgpack_unpacker *unpacker = NULL;
//#define DEBUG_MESSAGE     // uncomment for message debugging, will print out the binary message in hex

/*!
 * Event handler for all HTTP events.
 * This function is called automatically from the system,
 * @param evt, which calls this handler
 * @return error
 */
esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
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
            if (!strcmp("Content-Length", (const char *) (evt->header_key))) {
                ESP_LOGD(TAG, "MESSAGE LENGTH %s", evt->header_value);
            }
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // Write out data
//                printf("%.*s", evt->data_len, (char *) evt->data);
#ifdef DEBUG_MESSAGE
                print_message(evt->data, evt->data_len);
#endif /* DEBUG_MESSAGE */
                if (msgpack_unpacker_buffer_capacity(unpacker) < evt->data_len) {
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
    unpacker = msgpack_unpacker_new(rcv_buffer_size);

    esp_http_client_config_t config = {
            .url = url,
            .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    // POST
    esp_http_client_set_url(client, url);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/octet-stream");
    esp_http_client_set_header(client, "Authorization", UBIRCH_AUTH);
    esp_http_client_set_post_field(client, data, (int) (length));
    esp_err_t err = esp_http_client_perform(client);
    bool success = false;
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
        success = (esp_http_client_get_status_code(client) >= 200 && esp_http_client_get_status_code(client) <= 299);
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    if (success) {
        int size_unpacker = msgpack_unpacker_message_size(unpacker);
        ESP_LOGI(TAG, "unpacker-size %d", size_unpacker);
        check_response();
    }
    msgpack_unpacker_free(unpacker);
}


void check_response() {
    ESP_LOGI(TAG, "check");
    if (!ubirch_protocol_verify(unpacker, esp32_ed25519_verify)) {
        ESP_LOGI(TAG, "check successful");
        parse_payload(unpacker);
    } else {
        ESP_LOGW(TAG, "verification failed");
    }
}


void parse_payload(msgpack_unpacker *unpacker) {
    ESP_LOGI(TAG, "parsing payload");
    // new unpacked result buffer
    msgpack_unpacked result;
    msgpack_unpacked_init(&result);
    // unpack into result buffer and look for ARRAY
    if (msgpack_unpacker_next(unpacker, &result) && result.data.type == MSGPACK_OBJECT_ARRAY) {
        // redirect the result to the envelope
        msgpack_object *envelope = result.data.via.array.ptr;
        int p_version = 0;
        // get the envelope version
        if (envelope->type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            p_version = (int) envelope->via.u64;
            ESP_LOGI(TAG, "VERSION: %d (variant %d)\r\n", p_version >> 4, p_version & 0xf);
        }
        // get the backend UUID
        if ((++envelope)->type == MSGPACK_OBJECT_RAW) {
            ESP_LOGI(TAG, "UUID: ");
            // print_message(envelope->via.raw.ptr, envelope->via.raw.size);
        }
        // only continue if the envelope version and variant match
        if (p_version == proto_chained) {
            // previous message signature (from our request message)
            unsigned char prev_signature[UBIRCH_PROTOCOL_SIGN_SIZE];
            if (load_signature(prev_signature)) {
                ESP_LOGW(TAG, "error loading signature");
            }
            // compare the previous signature to the received one
            bool prevSignatureMatches = false;
            if ((++envelope)->type == MSGPACK_OBJECT_RAW) {
#ifdef DEBUG_MESSAGE
                print_message(envelope->via.raw.ptr, envelope->via.raw.size);
#endif /* DEBUG_MESSAGE */
                if (envelope->via.raw.size == crypto_sign_BYTES) {
                    prevSignatureMatches = !memcmp(prev_signature, envelope->via.raw.ptr, UBIRCH_PROTOCOL_SIGN_SIZE);
                }
            }
            // only continue, if the signatures match
            if (prevSignatureMatches && (++envelope)->type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
                ESP_LOGI(TAG, "TYPE: %d\r\n", (unsigned int) envelope->via.u64);
                switch ((unsigned int) envelope->via.u64) {
                    case MSGPACK_MSG_REPLY:
                        parse_measurement_reply(++envelope);
                        break;
                    case UBIRCH_PROTOCOL_TYPE_HSK:
                        //TODO handshake reply evaluation
                        break;
                    default:
                        ESP_LOGI(TAG, "unknown packet data type");
                        break;
                }
            } else {
                ESP_LOGW(TAG, "prev signature mismatch or message type wrong!");
            }
        } else {
            ESP_LOGW(TAG, "protocol version mismatch: %d != %d", p_version, proto_chained);
        }
    } else {
        ESP_LOGW(TAG, "empty message not accepted");
    }
    ESP_LOGI(TAG, "destroy result");
    msgpack_unpacked_destroy(&result);
}

int response = 0;


void parse_measurement_reply(msgpack_object *envelope) {
    ESP_LOGI(TAG, "measurement reply");
    if (envelope->type == MSGPACK_OBJECT_MAP) {
        msgpack_object_kv *map = envelope->via.map.ptr;
        for (uint32_t entries = 0; entries < envelope->via.map.size; map++, entries++) {
            //
            // UI PARAMETER SECTION
            // ad or modify the used parameters here
            //
            if (match(map, "i", MSGPACK_OBJECT_POSITIVE_INTEGER)) {
                // read new interval setting
                unsigned int value = (unsigned int) (map->val.via.u64);
                ESP_LOGI(TAG, "i = %d ", value);
                response = value;
            }
            //
            // END OF PARAMETER SECTION
            //
            else {
                ESP_LOGI(TAG, "unknown MSGPACK object in MAP");
            }
        }
    } else {
        ESP_LOGI(TAG, "unknown MSGPACK object");
    }
}


bool match(const msgpack_object_kv *map, const char *key, const int type) {
    ESP_LOGI(TAG, "match");
    const size_t keyLength = strlen(key);
    return map->key.type == MSGPACK_OBJECT_RAW &&
           map->key.via.raw.size == keyLength &&
           (type == -1 || map->val.type == type) &&
           !memcmp(key, map->key.via.raw.ptr, keyLength);
}


void create_message(void) {
    // create buffer, writer, ubirch protocol context and packer
    msgpack_sbuffer *sbuf = msgpack_sbuffer_new();
    ubirch_protocol *proto = ubirch_protocol_new(proto_chained, MSGPACK_MSG_UBIRCH,
                                                 sbuf, msgpack_sbuffer_write, esp32_ed25519_sign, UUID);
    msgpack_packer *pk = msgpack_packer_new(proto, ubirch_protocol_write);
    // load the old signature and copy it to the protocol
    unsigned char old_signature[UBIRCH_PROTOCOL_SIGN_SIZE] = {};
    if (load_signature(old_signature)) {
        ESP_LOGW(TAG, "error loading the old signature");
    }
    memcpy(proto->signature, old_signature, UBIRCH_PROTOCOL_SIGN_SIZE);
    // start the protocol
    ubirch_protocol_start(proto, pk);
    //
    // PAYLOAD SECTION
    // add or modify the data here
    //
    // create array[ timestamp, value ])
    msgpack_pack_array(pk, 2);
    uint64_t ts = get_time_us();
    msgpack_pack_uint64(pk, ts);
    uint32_t fake_temp = (esp_random() & 0x0F);
    msgpack_pack_int32(pk, (int32_t) (fake_temp));
    //
    // END OF PAYLOAD
    //
    // finish the protocol
    ubirch_protocol_finish(proto, pk);
    if (store_signature(proto->signature, UBIRCH_PROTOCOL_SIGN_SIZE)) {
        ESP_LOGW(TAG, "error storing the signature");
    }
#ifdef DEBUG_MESSAGE
    print_message((const char *) (sbuf->data), (size_t) (sbuf->size));
#endif /* DEBUG_MESSAGE */
    // send the message
    http_post_task(UHTTP_URL, (const char *) (sbuf->data), sbuf->size);

    ESP_LOGI(TAG, "cleaning up");
    // free allocated ressources
    msgpack_packer_free(pk);
    ubirch_protocol_free(proto);
    msgpack_sbuffer_free(sbuf);
}
