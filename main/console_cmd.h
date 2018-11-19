/*!
 * @file    console_cmd.h
 * @brief   Custom Console commands header file.
 *
 * @author Waldemar Gruenwald
 * @date   2018-11-14
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

#ifndef EXAMPLE_ESP32_CONSOLE_CMD_H
#define EXAMPLE_ESP32_CONSOLE_CMD_H

// exit code from console
#define EXIT_CONSOLE 0xFF

// Register system functions
void register_system();

// Register WiFi functions
void register_wifi();

// Register the regular program
void register_exit();

// Register the status
void register_status();


#endif //EXAMPLE_ESP32_CONSOLE_CMD_H
