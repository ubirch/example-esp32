/*!
 * @file    wifi.h
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

#ifndef WIFI_H
#define WIFI_H

struct Wifi_login {
    char ssid[64];
    size_t ssid_length;
    char pwd[64];
    size_t pwd_length;
};

/*!
 * Initialize the wifi module and connect to the provided SSID.
 */
void my_wifi_init(void);

/*!
 * Load the wifi login data from flash.
 *
 * @param ssid
 * @param pwd
 * @return true, if error occured,
 * @return false, if the login data was loaded successfully
 */
//bool load_wifi_login(struct Wifi_login wifi);

bool wifi_join(struct Wifi_login wifi, int timeout_ms);

#endif /* WIFI_H */