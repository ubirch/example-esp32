/*!
 * @file
 * @brief ubirch firmware update
 *
 * ...
 *
 * @author Matthias L. Jugel
 * @date   2018-12-01
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
#ifndef FIRMWARE_UPDATE_H
#define FIRMWARE_UPDATE_H

#define FIRMWARE_UPDATE_CHECK_INTERVAL 3600000

/**
 * Task that takes care of regular checks for new firmware.
 *
 * @param pvParameters
 */
void firmware_update_task(void *pvParameters);

#endif //FIRMWARE_UPDATE_H
