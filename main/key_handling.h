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



#ifndef KEY_HANDLING_H
#define KEY_HANDLING_H

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif


/*!  
 * Create a new signature Key pair.
 *
 * After creating the key pair, it is packad into msgpack together with aditional
 * information, according to the structure `ubirch_key_info()`, from `ubirch_protocol_kex.h`,
 * which is part of the `ubirch-protocol` module.
 */
void create_keys(void);

/*!
 * Register the Keys in the backend.
 *
 * This function can only be executed, if a network connection is available.
 */
void register_keys(void);

/*!
 * Check the current key status.
 *
 * If no keys are present, create new keys. The key registration has to be executed separately.
 */
void check_key_status(void);

#ifdef __cplusplus
}
#endif

#endif /* KEY_HANDLING_H */
