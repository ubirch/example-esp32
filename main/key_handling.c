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
#include <stdbool.h>
#include <esp_err.h>
#include <nvs.h>
#include <time.h>
#include <esp_log.h>

#include "ubirch_ed25519.h"
#include "ubirch_protocol_kex.h"

#include "util.h"
#include "ubirch-proto-http.h"
#include "settings.h"
#include "key_handling.h"

static const char *TAG = "KEY_HANDLING";


extern unsigned char UUID[16];

unsigned char ed25519_secret_key[crypto_sign_SECRETKEYBYTES] = {};
unsigned char ed25519_public_key[crypto_sign_PUBLICKEYBYTES] = {};

const unsigned char server_pub_key[crypto_sign_PUBLICKEYBYTES] = {
        0xa2, 0x40, 0x3b, 0x92, 0xbc, 0x9a, 0xdd, 0x36,
        0x5b, 0x3c, 0xd1, 0x2f, 0xf1, 0x20, 0xd0, 0x20,
        0x64, 0x7f, 0x84, 0xea, 0x69, 0x83, 0xf9, 0x8b,
        0xc4, 0xc8, 0x7e, 0x0f, 0x4b, 0xe8, 0xcd, 0x66
};


/*
 * create a new signature Key pair
 */
void create_keys(void) {
    ESP_LOGI(TAG, "create keys");
    if (0) {}
    crypto_sign_keypair(ed25519_public_key, ed25519_secret_key);
    ESP_LOGI(TAG, "secretKey");
    print_message((const char *) (ed25519_secret_key), crypto_sign_SECRETKEYBYTES);
    ESP_LOGI(TAG, "publicKey");
    print_message((const char *) (ed25519_public_key), crypto_sign_PUBLICKEYBYTES);
}

/*
 * check for possible memory handling errors
 *
 * return error: true, if memory error, false otherwise
 */
bool memory_error_check(esp_err_t err) {
    bool error = true;
    switch (err) {
        case ESP_OK:
            error = false;
            break;
        case ESP_ERR_NVS_INVALID_HANDLE:
            ESP_LOGW(TAG, "memory invalid handle");
            break;
        case ESP_ERR_NVS_READ_ONLY:
            ESP_LOGW(TAG, "memory read only");
            break;
        case ESP_ERR_NVS_INVALID_NAME:
            ESP_LOGW(TAG, "memory invalid name");
            break;
        case ESP_ERR_NVS_NOT_ENOUGH_SPACE:
            ESP_LOGW(TAG, "memory not enoug space");
            break;
        case ESP_ERR_NVS_REMOVE_FAILED:
            ESP_LOGW(TAG, "memory remove failed");
            break;
        case ESP_ERR_NVS_VALUE_TOO_LONG:
            ESP_LOGW(TAG, "memory value too long");
            break;
        default:
            ESP_LOGW(TAG, "no handle for %d", err);
            break;
    }
    return error;
}


/*
 * Read the Key values from memory
 *
 * return error, true, if something went wrong, false if keys are available
 */
bool load_keys(void) {
    ESP_LOGI(TAG, "read keys");
    nvs_handle keyHandle;
    // open the memory
    esp_err_t err = nvs_open("key_storage", NVS_READONLY, &keyHandle);
    if (memory_error_check(err))
        return true;
    // read the secret key
    size_t size_sk = sizeof(ed25519_secret_key);
    err = nvs_get_blob(keyHandle, "secret_key", ed25519_secret_key, &size_sk);
    if (memory_error_check(err))
        return true;

    // read the public key
    size_t size_pk = sizeof(ed25519_public_key);
    err = nvs_get_blob(keyHandle, "public_key", ed25519_public_key, &size_pk);
    if (memory_error_check(err))
        return true;

    // close the memory
    nvs_close(keyHandle);
    return false;
}

/*
 * Write the key values to the memory
 *
 * return error: true, if something went wrong, false if keys were successfully stored
 */
bool store_keys(void) {
    ESP_LOGI(TAG, "write keys");
    nvs_handle keyHandle;
    // open the memory
    esp_err_t err = nvs_open("key_storage", NVS_READWRITE, &keyHandle);
    if (memory_error_check(err))
        return true;

    // read the secret key
    size_t size_sk = sizeof(ed25519_secret_key);
    err = nvs_set_blob(keyHandle, "secret_key", ed25519_secret_key, size_sk);
    if (memory_error_check(err))
        return true;

    // read the public key
    size_t size_pk = sizeof(ed25519_public_key);
    err = nvs_set_blob(keyHandle, "public_key", ed25519_public_key, size_pk);
    if (memory_error_check(err))
        return true;

    // ensure the changes are written to the memory
    err = nvs_commit(keyHandle);
    if (memory_error_check(err))
        return true;

    // close the memory
    nvs_close(keyHandle);
    return false;
}

/*
 * Load the last signature
 *
 * return error: true, if something went wrong, false if keys were successfully stored
 */
bool load_signature(unsigned char *signature) {
    ESP_LOGI(TAG, "load signature");
    nvs_handle signatureHandle;
    // open the memory
    esp_err_t err = nvs_open("sign_storage", NVS_READONLY, &signatureHandle);
    if (memory_error_check(err))
        return true;
    // read the last signature
    size_t size_sig = UBIRCH_PROTOCOL_SIGN_SIZE;
    err = nvs_get_blob(signatureHandle, "signature", signature, &size_sig);
    if (memory_error_check(err))
        return true;

    // close the memory
    nvs_close(signatureHandle);
    return false;
}

/*
 * Load the last signature
 */
bool store_signature(const unsigned char *signature, size_t size_sig) {
    ESP_LOGI(TAG, "store signature");
    nvs_handle signatureHandle;
    // open the memory
    esp_err_t err = nvs_open("sign_storage", NVS_READWRITE, &signatureHandle);
    if (memory_error_check(err))
        return true;
    // read the last signature
    err = nvs_set_blob(signatureHandle, "signature", signature, size_sig);
    if (memory_error_check(err))
        return true;

    // close the memory
    nvs_close(signatureHandle);
    return false;
}


void register_keys(void) {
    ESP_LOGI(TAG, "register keys");
    // create buffer, protocol and packer
    msgpack_sbuffer *sbuf = msgpack_sbuffer_new();
    ubirch_protocol *proto = ubirch_protocol_new(proto_signed, UBIRCH_PROTOCOL_TYPE_REG,
                                                 sbuf, msgpack_sbuffer_write, ed25519_sign, UUID);
    msgpack_packer *pk = msgpack_packer_new(proto, ubirch_protocol_write);
    // start the ubirch protocol message
    ubirch_protocol_start(proto, pk);
    // create key registration info
    ubirch_key_info info = {};
    info.algorithm = (char *) (UBIRCH_KEX_ALG_ECC_ED25519);
    info.created = time(NULL);                                  // current time of the system
    memcpy(info.hwDeviceId, UUID, sizeof(UUID));                // 16 Byte unique hardware device ID
    memcpy(info.pubKey, ed25519_public_key, sizeof(ed25519_public_key));    // the public key
    info.validNotAfter = time(NULL) + 31536000;                 // time until the key will be valid (now + 1 year)
    info.validNotBefore = time(NULL);                           // time from when the key will be valid (now)
    // pack the key registration msgpack
    msgpack_pack_key_register(pk, &info);
    // finish the ubirch protocol message
    ubirch_protocol_finish(proto, pk);
    // send the data
    http_post_task(UKEY_SERVICE_URL, sbuf->data, sbuf->size);
    // free allocated ressources
    msgpack_packer_free(pk);
    ubirch_protocol_free(proto);
    msgpack_sbuffer_free(sbuf);
}

void check_key_status(void) {
    //read the Keys, if available
    if (load_keys()) {
        create_keys();
        store_keys();
        register_keys();
    }
}


int esp32_ed25519_sign(const unsigned char *data, size_t len, unsigned char *signature) {
    return ed25519_sign_key(data, len, signature, ed25519_secret_key);
}

int esp32_ed25519_verify(const unsigned char *data, size_t len, const unsigned char *signature) {
    return ed25519_verify_key(data, len, signature, server_pub_key);
}

