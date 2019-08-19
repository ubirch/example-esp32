//
// Created by wowa on 21.05.19.
//

#ifndef EXAMPLE_ESP32_UBIRCH_PROTOCOL_C8Y_H
#define EXAMPLE_ESP32_UBIRCH_PROTOCOL_C8Y_H

#include <ubirch_protocol.h>

/**
 * ubirch protocol context, which holds the underlying packer, the uuid and current and previous signature
 * as well as the current hash.
 */
typedef struct ubirch_protocol_c8y {
	void *data;                                         //!< the data
	ubirch_protocol_sign sign;                          //!< the message signing function
	uint16_t version;                                   //!< the specific used protocol version
	unsigned int type;                                  //!< the payload type (0 - unspecified, app specific)
	unsigned char uuid[UBIRCH_PROTOCOL_UUID_SIZE];      //!< the uuid of the sender (used to retrieve the keys)
	unsigned char signature[UBIRCH_PROTOCOL_SIGN_SIZE]; //!< the current or previous signature of a message
	mbedtls_sha512_context hash;                        //!< the streaming hash of the data to sign
	unsigned int status;                                //!< amount of bytes packed
} ubirch_protocol_c8y;


typedef struct c8y_response {
	char *buffer;
	size_t used;
	size_t free;
} c8y_response;

/*!
 * Initialize the cumulocity http client.
 * @param uuid id of the device
 */
void c8y_http_client_init(const char *uuid);

/*!
 * Perform a measurement post to Cumulocity.
 *
 * @param timestamp of the current measurement
 * @param temperature floating point value of the temperature
 * @param humidity floating point value of the humidity
 * @return ESP_OK, if measurement was posted successfully
 * @return ESP_FAIL, if error
 */
esp_err_t c8y_measurement(time_t timestamp, float temperature, float humidity);

/*!
 * Create a json object for the measurement data to post to Cumulocity.
 *
 * @param timestamp of the current measurement
 * @param f_temperature floating point value of the temperature
 * @param f_humidity floating point value of the humidity
 * @return serialized json in char string format
 */
char *c8y_measurement_create_json(time_t timestamp, float f_temperature, float f_humidity);

/*!
 * Get the authorization in base64 encoding,
 * consisting of the username and the password
 *
 * @param auth is the pointer to the string pointer for the authorization string.
 *
 * @note this function allocates memory for the authorization token.
 * todo error handling
 */
void c8y_get_authorization(char **auth);

/*!
 * Start the cumulocity client
 */
void c8y_start(void);


/*!
 * TODO
 * @param proto
 * @param variant
 * @param data_type
 * @param data
 * @param sign
 * @param uuid
 */
inline void ubirch_protocol_c8y_init(ubirch_protocol_c8y *proto,
                                     enum ubirch_protocol_variant variant,
                                     unsigned int data_type,
                                     void *data,
                                     ubirch_protocol_sign sign,
                                     const unsigned char uuid[UBIRCH_PROTOCOL_UUID_SIZE]) {
	proto->data = data;
	proto->sign = sign;
	proto->hash.is384 = -1;
	proto->version = variant;
	proto->type = data_type;
	memcpy(proto->uuid, uuid, UBIRCH_PROTOCOL_UUID_SIZE);
	proto->status = UBIRCH_PROTOCOL_INITIALIZED;
}


/*!
 * TODO
 * @param variant
 * @param data_type
 * @param data
 * @param callback
 * @param sign
 * @param uuid
 * @return
 */
inline ubirch_protocol_c8y *ubirch_protocol_c8y_new(enum ubirch_protocol_variant variant,
                                                    unsigned int data_type,
                                                    void *data,
                                                    ubirch_protocol_sign sign,
                                                    const unsigned char uuid[UBIRCH_PROTOCOL_UUID_SIZE]) {
	ubirch_protocol_c8y *proto = (ubirch_protocol_c8y *) calloc(1, sizeof(ubirch_protocol_c8y));
	if (!proto) { return NULL; }
	ubirch_protocol_c8y_init(proto, variant, data_type, data, sign, uuid);

	return proto;
}


#endif //EXAMPLE_ESP32_UBIRCH_PROTOCOL_C8Y_H
