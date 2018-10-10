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
 * Create a new signature Key pair
 */
void createKeys(void);

/*!
 * Check for possible memory handling errors
 *
 * @return true, if memory error,
 * @return false if no error
 */
bool memoryErrorCheck(esp_err_t err);

/*!
 * Read the Key values from memory
 *
 * @return true, if something went wrong,
 * @return false if keys are available
 */
bool readKeys(void);

/*!
 * Write the key values to the memory
 *
 * @return true, if something went wrong,
 * @return false if keys were successfully stored
 */
bool writeKeys(void);

/*!
 * Load the last signature
 *
 * @param   signature pointer to signature array
 *
 * @return  true, if something went wrong,
 * @return  false if keys were successfully stored
 */
bool loadSignature(unsigned char *signature);

/*!
 * Store the last signature.
 *
 * @param signature pointer to signature array
 * @param size_sig size of the signature array
 *
 * @return error: true if signature could not be stored, false otherwise
 */
bool storeSignature(const unsigned char *signature, size_t size_sig);

/*!
 * Register the Keys in the backend.
 */
void registerKeys(void);

/*!
 * Check the current key status.
 * If no keys are present, create new keys and register them at the backend
 */
void checkKeyStatus(void);


/*!
 * Helper function to forward the required parameters to the signing function.
 * The signing function will sign the payload and return the signature.
 *
 * @param data the buffer with the data to sign
 * @param len the length of the data buffer
 * @param signature the buffer to hold the returned signature
 *
 * @return 0 on success
 * @return -1 if the signing failed
 */

int esp32_ed25519_sign(const unsigned char *data, size_t len, unsigned char *signature);

/*!
 * Helper function to forward the required parameters to the verification function.
 * The verification function will verify a data buffer with the provided signature.
 *
 * @param data the buffer with the data to verify
 * @param len the length of the data buffer
 * @param signature a buffer with the corresponding signature
 *
 * @return 0 on success
 * @return -1 if the verification failed
 */
int esp32_ed25519_verify(const unsigned char *data, size_t len, const unsigned char *signature);


#ifdef __cplusplus
}
#endif

#endif /* KEY_HANDLING_H */