/*!
 * @file    http.h
 * @brief   http functions to send ubirch protocol messages via http post
 *          and to evaluate the response.
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
 *
 * The code was taken from:
 *
 * ESP HTTP Client Example
 *
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 *
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 */
#ifndef _HTTP_H_
#define _HTTP_H_

#define MSGPACK_MSG_REPLY 85
#define MSGPACK_MSG_UBIRCH 50

/*!
 * initialize the unpacker
 */
void http_init_unpacker(void);

/*!
 * Perform a http post request with the given data.
 *
 * @param url       destination url for the post
 * @param data      data buffer
 * @param length    length of the data buffer
 */
void http_post_task(const char *url, const char *data, const size_t length);

/*!
 * Check the response of the http post by verification of the signature
 */
void check_response(void);

/*!
 * Parse the payload of the message reply
 * @param unpacker received data
 */
void parse_payload(msgpack_unpacker *unpacker);

/*!
 * Parse a measurement packet reply.
 * @param envelope the packet envelope at the point where the payload starts
 */
void parseMeasurementReply(msgpack_object *envelope);

/*!
 * Helper function, checking a specific key in a msgpack map.
 * @param map the map
 * @param key the key we look for
 * @param type the type of the value we look for
 * @return true if the key and type match
 */
bool match(const msgpack_object_kv *map, const char *key, const int type);

/*!
 * create a message with msgpack and ubirch protocol and send it
 */
void create_message(void);

#endif /*_HTTP_H_*/

