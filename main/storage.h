//
// Created by wowa on 14.11.18.
//

#ifndef EXAMPLE_ESP32_STORAGE_H
#define EXAMPLE_ESP32_STORAGE_H

#include <stdbool.h>

bool store_wifi_login(const char *ssid, size_t size_ssid, const char *pwd, size_t size_pwd);

bool load_wifi_login(char *ssid, char *pwd);
/*!
 * Load the last signature
 *
 * @param   signature pointer to signature array
 *
 * @return  true, if something went wrong,
 * @return  false if keys were successfully stored
 */
bool load_signature(unsigned char *signature);

/*!
 * Store the last signature.
 *
 * @param signature pointer to signature array
 * @param size_sig size of the signature array
 *
 * @return error: true if signature could not be stored, false otherwise
 */
bool store_signature(const unsigned char *signature, size_t size_sig);


#endif //EXAMPLE_ESP32_STORAGE_H
