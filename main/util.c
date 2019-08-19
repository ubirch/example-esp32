/*!
 * @file    util.h
 * @brief   Utility functions
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
#include <esp_log.h>
#include <esp_system.h>
#include <storage.h>
#include <string.h>

#include "util.h"

unsigned char UUID[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x23,
                          0x34, 0x45, 0x56, 0x67, 0x78, 0x89, 0x9a, 0xab
};


void set_hw_ID(void) {
    ESP_LOGI(__func__, "");
    esp_efuse_mac_get_default(UUID);
    esp_base_mac_addr_set(UUID);
    kv_store("device-status", "hw-dev-id", UUID, sizeof(UUID));
}

void get_hw_ID(void) {
	printf("Hardware-Device-ID: %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x\r\n",
	       UUID[0], UUID[1], UUID[2], UUID[3], UUID[4], UUID[5], UUID[6], UUID[7],
	       UUID[8], UUID[9], UUID[10], UUID[11], UUID[12], UUID[13], UUID[14], UUID[15]);

}

char *get_hw_ID_string(void) { // 37 char
	char *string_UUID = malloc(2 * sizeof(UUID) + 5);
	sprintf(string_UUID, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
	        UUID[0], UUID[1], UUID[2], UUID[3], UUID[4], UUID[5], UUID[6], UUID[7],
	        UUID[8], UUID[9], UUID[10], UUID[11], UUID[12], UUID[13], UUID[14], UUID[15]);
	return string_UUID;
}
