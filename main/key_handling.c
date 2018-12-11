/*!
 * @file
 * @brief key and signature helping functions
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


#include <stdio.h>
#include <esp_err.h>
#include <time.h>
#include <esp_log.h>
#include <storage.h>
#include <ubirch_api.h>

#include "ubirch_ed25519.h"
#include "ubirch_protocol_kex.h"

#include "key_handling.h"

static const char *TAG = "KEYSTORE";

extern unsigned char UUID[16];

unsigned char ed25519_secret_key[crypto_sign_SECRETKEYBYTES] = {};
unsigned char ed25519_public_key[crypto_sign_PUBLICKEYBYTES] = {};

const unsigned char server_pub_key[crypto_sign_PUBLICKEYBYTES] = {
        0xa2, 0x40, 0x3b, 0x92, 0xbc, 0x9a, 0xdd, 0x36,
        0x5b, 0x3c, 0xd1, 0x2f, 0xf1, 0x20, 0xd0, 0x20,
        0x64, 0x7f, 0x84, 0xea, 0x69, 0x83, 0xf9, 0x8b,
        0xc4, 0xc8, 0x7e, 0x0f, 0x4b, 0xe8, 0xcd, 0x66
};


/*!
 * Read the Key values from memory
 *
 * return error, true, if something went wrong, false if keys are available
 */
static esp_err_t load_keys(void) {
    ESP_LOGI(TAG, "read keys");
    esp_err_t err;

    unsigned char *key = ed25519_secret_key;

    // read the secret key
    size_t size_sk = sizeof(ed25519_secret_key);
    err = kv_load("key_storage", "secret_key", (void **) &key, &size_sk);
    if (memory_error_check(err)) return err;

    // read the public key
    key = ed25519_public_key;
    size_t size_pk = sizeof(ed25519_public_key);
    err = kv_load("key_storage", "public_key", (void **) &key, &size_pk);
    if (memory_error_check(err)) return err;

    return err;
}


/*!
 * Write the key values to the memory
 *
 * @return error: true, if something went wrong, false if keys were successfully stored
 */
static esp_err_t store_keys(void) {
    ESP_LOGI(TAG, "write keys");
    esp_err_t err;
    // store the secret key
    err = kv_store("key_storage", "secret_key", ed25519_secret_key, sizeof(ed25519_secret_key));
    if (memory_error_check(err)) return err;
    //store the public key
    err = kv_store("key_storage", "public_key", ed25519_public_key, sizeof(ed25519_public_key));
    if (memory_error_check(err)) return err;

    return err;
}


/*!
 * Create a new signature Key pair
 */
void create_keys(void) {
    ESP_LOGI(TAG, "create keys");
    // create the key pair
    crypto_sign_keypair(ed25519_public_key, ed25519_secret_key);
    ESP_LOGD(TAG, "publicKey");
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, (const char *) (ed25519_public_key), crypto_sign_PUBLICKEYBYTES, ESP_LOG_DEBUG);

    // create the certificate for the key pair
    msgpack_sbuffer *sbuf = msgpack_sbuffer_new();
    // create buffer, protocol and packer
    ubirch_protocol *proto = ubirch_protocol_new(proto_signed, UBIRCH_PROTOCOL_TYPE_REG,
                                                 sbuf, msgpack_sbuffer_write, ed25519_sign, UUID);
    msgpack_packer *pk = msgpack_packer_new(proto, ubirch_protocol_write);
    // start the ubirch protocol message
    ubirch_protocol_start(proto, pk);
    // create key registration info
    ubirch_key_info info = {};
    info.algorithm = (char *) (UBIRCH_KEX_ALG_ECC_ED25519);
    info.created = (unsigned int) time(NULL);                           // current time of the system
    memcpy(info.hwDeviceId, UUID, sizeof(UUID));                        // 16 Byte unique hardware device ID
    memcpy(info.pubKey, ed25519_public_key, sizeof(ed25519_public_key));// the public key
    info.validNotAfter = (unsigned int) (time(NULL) + 31536000);        // time until the key will be valid (now + 1 year)
    info.validNotBefore = (unsigned int) time(NULL);                    // time from when the key will be valid (now)
    // pack the key registration msgpack
    msgpack_pack_key_register(pk, &info);
    // finish the ubirch protocol message
    ubirch_protocol_finish(proto, pk);
    // free allocated ressources
    msgpack_packer_free(pk);
    ubirch_protocol_free(proto);

    // store the generated certificate
    esp_err_t err = kv_store("key_storage", "certificate", sbuf->data, sbuf->size);
    if (err != ESP_OK){
        ESP_LOGW(TAG, "key certificate could not be stored in flash");
    }
    // store the keys
    if (store_keys() != ESP_OK){
        ESP_LOGW(TAG, "generated keys could not be stored in flash");
    }
    // free the allocated resources
    msgpack_sbuffer_free(sbuf);
}


void register_keys(void) {
    ESP_LOGI(TAG, "register identity");

    msgpack_sbuffer *sbuf = msgpack_sbuffer_new();
    // try to load the certificate if it was generated and stored before
    esp_err_t err = kv_load("key_storage", "certificate", (void **) &sbuf->data, &sbuf->size);
    if(err != ESP_OK) {
        ESP_LOGW(TAG, "creating new certificate");
        create_keys();
    } else {
        ESP_LOGI(TAG, "loaded certificate");
    }

    // send the data
    err = ubirch_send(CONFIG_UBIRCH_BACKEND_KEY_SERVER_URL, sbuf->data, sbuf->size, NULL);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "unable to send registration");
    }
    msgpack_sbuffer_free(sbuf);
    ESP_LOGI(TAG, "successfull sent registration");
}


void check_key_status(void) {
    if (load_keys() != ESP_OK) {
        create_keys();
    }
}