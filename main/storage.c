//
// Created by wowa on 14.11.18.
//

#include <esp_log.h>
#include <nvs.h>
#include <ubirch_protocol.h>
#include "key_handling.h"
#include "storage.h"

static const char *TAG = "storage";

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


bool store_wifi_login(const char *ssid, size_t size_ssid, const char *pwd, size_t size_pwd) {
    nvs_handle wifiHandle;
    // open the memory
    esp_err_t err = nvs_open("wifi_data", NVS_READWRITE, &wifiHandle);
    memory_error_check(err);
    // erase old data
    err = nvs_erase_all(wifiHandle);
    memory_error_check(err);
    err = nvs_commit(wifiHandle);
    memory_error_check(err);
    nvs_close(wifiHandle);

    err = nvs_open("wifi_data", NVS_READWRITE, &wifiHandle);

    // store the wifi ssid length
    err = nvs_set_u32(wifiHandle, "wifi_ssid_l", size_ssid);
    memory_error_check(err);
    // store the wifi ssid
    err = nvs_set_blob(wifiHandle, "wifi_ssid", ssid, size_ssid);
    memory_error_check(err);
    // store the wifi pwd length
    err = nvs_set_u32(wifiHandle, "wifi_pwd_l", size_pwd);
    memory_error_check(err);
    // store the wifi pwd
    err = nvs_set_blob(wifiHandle, "wifi_pwd", pwd, size_pwd);
    memory_error_check(err);
    // close the memory
    nvs_close(wifiHandle);
    ESP_LOGI(TAG, "stored login data");
    if (memory_error_check(err))
        return true;
    else {
        return false;
    }
}
/*!
 * Load the wifi login data from flash.
 *
 * @param ssid
 * @param pwd
 * @return true, if error occured,
 * @return false, if the login data was loaded successfully
 */
bool load_wifi_login(char *ssid, char *pwd) {
    ESP_LOGI(TAG, "load login data");
    nvs_handle wifiHandle;
    // open the memory
    esp_err_t err = nvs_open("wifi_data", NVS_READONLY, &wifiHandle);
    memory_error_check(err);

    size_t size_ssid = 0;
    // load the wifi ssid length
    err = nvs_get_u32(wifiHandle, "wifi_ssid_l", &size_ssid);
    memory_error_check(err);
    // load the wifi ssid
    err = nvs_get_blob(wifiHandle, "wifi_ssid", ssid, &size_ssid);
    memory_error_check(err);
    size_t size_pwd = 0;
    // load the wifi pwd length
    err = nvs_get_u32(wifiHandle, "wifi_pwd_l", &size_pwd);
    memory_error_check(err);
    // load the wifi pwd
    err = nvs_get_blob(wifiHandle, "wifi_pwd", pwd, &size_pwd);
    memory_error_check(err);
    // close the memory
    nvs_close(wifiHandle);
    if (memory_error_check(err))
        return true;
    else {
        return false;
    }
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

