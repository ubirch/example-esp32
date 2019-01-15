/*!
 * @file
 * @brief sensor handling, for user code
 *
 * ...
 *
 * @author Matthias L. Jugel
 * @date   2018-12-05
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

#include <freertos/FreeRTOS.h>
#include <msgpack.h>
#include <message.h>
#include <ubirch_api.h>
#include <response.h>
#include <esp_log.h>
#include <esp32-hal-adc.h>
#include "sensor.h"

#define BLUE_LED GPIO_NUM_2
#define BOOT_BUTTON GPIO_NUM_0

extern unsigned char UUID[16];

TickType_t interval = CONFIG_UBIRCH_DEFAULT_INTERVAL;

/*!
 * This function handles responses from the backend, where we can set parameters.
 * @param entry a msgpack entry as received
 */
void response_handler(const struct msgpack_object_kv *entry) {
    if (match(entry, "i", MSGPACK_OBJECT_POSITIVE_INTEGER)) {
        interval = (unsigned int) (entry->val.via.u64);
    } else {
        ESP_LOGW(__func__, "unknown configuration received: %.*s", entry->key.via.raw.size, entry->key.via.raw.ptr);
    }
}

/*!
 * The actual sending function that packages the data and sends it to the backend.
 * @param temperature the temperature to send
 * @param humidity the humidity value to send
 * @return ESP_OK or an error code
 */
static esp_err_t send_message(float temperature, float humidity) {
    msgpack_sbuffer *sbuf = msgpack_sbuffer_new(); //!< send buffer
    msgpack_unpacker *unpacker = msgpack_unpacker_new(128); //!< receive unpacker

    int32_t values[2] = {(int32_t) (temperature * 100), (int32_t) (humidity * 100)};
    ubirch_message(sbuf, UUID, values, sizeof(values) / sizeof(values[0]));

    ubirch_send(CONFIG_UBIRCH_BACKEND_DATA_URL, sbuf->data, sbuf->size, unpacker);
    ubirch_parse_response(unpacker, response_handler);

    msgpack_unpacker_free(unpacker);
    msgpack_sbuffer_free(sbuf);

    return ESP_OK;
}

void sensor_setup() {
    // set the blue LED pin on the ESP32 DEVKIT V1 board
    gpio_set_direction(BLUE_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(BOOT_BUTTON, GPIO_MODE_INPUT);
}

/*!
 * The main sensor measurement function.
 */
 void sensor_loop() {
     // let the LED blink
    if (interval < 1000) gpio_set_level(BLUE_LED, 0); else gpio_set_level(BLUE_LED, 1);

    int f_hall = hallRead();
    float f_temperature = temperatureRead();
    ESP_LOGI(__func__, "Hall Sensor = %d ", f_hall);
    ESP_LOGI(__func__, "Temp Sensor = %f", f_temperature);

    send_message(f_hall, f_temperature);

    vTaskDelay(pdMS_TO_TICKS(CONFIG_UBIRCH_DEFAULT_INTERVAL));
}