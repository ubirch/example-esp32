#ifndef _HTTP_H_
#define _HTTP_H_

#define MSGPACK_MSG_REPLY 85
#define MSGPACK_MSG_UBIRCH 83

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

