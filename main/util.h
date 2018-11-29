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


#ifndef UTIL_H
#define UTIL_H

/*!
 * Print out a data message in hex format to the console.
 *
 * @param data buffer for printing
 * @param size of the data buffer
 */
void print_message(const char *data, size_t size);

/*!
 * Set the Hardware-Device-ID, generated from the MAC address of the device.
 */
void set_hw_ID(void);

/*!
 * Get the hardware device ID and print it out
 */
void get_hw_ID(void);

#endif /* UTIL_H */