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
	size_t initial_buffer_size;
} c8y_response;

typedef unsigned int _c8y_atomic_counter_t;
#define _c8y_sync_decr_and_fetch(ptr) __sync_sub_and_fetch(ptr, 1)
#define _c8y_sync_incr_and_fetch(ptr) __sync_add_and_fetch(ptr, 1)


//bool c8y_expand_buffer(c8y_response *response, size_t size);

//bool c8y_response_init(c8y_response *response, size_t initial_buffer_size);


void c8y_http_client_init(const char *uuid);

esp_err_t c8y_measurement(time_t timestamp, float temperature, float humidity);

char *c8y_measurement_create_json(time_t timestamp, float f_temperature, float f_humidity);

void c8y_get_authorization(char **auth);


void c8y_test(void);


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

inline int ubirch_protocol_c8y_start(ubirch_protocol *proto) {
//	if (proto == NULL || pk == NULL) return -1;
//	if (proto->status != UBIRCH_PROTOCOL_INITIALIZED) return -2;
//
//	if (proto->version == proto_signed || proto->version == proto_chained) {
//		mbedtls_sha512_init(&proto->hash);
//		mbedtls_sha512_starts(&proto->hash, 0);
//	}
//
//	// the message consists of 3 header elements, the payload and (not included) the signature
//	switch (proto->version) {
//		case proto_plain:
//			// todo
//			break;
//		case proto_signed:
//			// todo
//			break;
//		case proto_chained:
//			// todo
//			break;
//		default:
//			return -3;
//	}
//
//	/*
//	 * +=========+======+==================+======+=========+-------------+
//	 * | VERSION | UUID | [PREV-SIGNATURE] | TYPE | PAYLOAD | [SIGNATURE] |
//	 * +=========+======+==================+======+=========+-------------+
//	 */
//
//	// 1 - protocol version
//	msgpack_pack_fix_uint16(pk, proto->version);
//
//	// 2 - device ID
//	msgpack_pack_raw(pk, 16);
//	msgpack_pack_raw_body(pk, proto->uuid, sizeof(proto->uuid));
//
//	// 3 the last signature (if chained)
//	if (proto->version == proto_chained) {
//		msgpack_pack_raw(pk, sizeof(proto->signature));
//		msgpack_pack_raw_body(pk, proto->signature, sizeof(proto->signature));
//	}
//
//	// 4 the payload type
//	msgpack_pack_int(pk, proto->type);
//
//	proto->status = UBIRCH_PROTOCOL_STARTED;
	return 0;
}


#endif //EXAMPLE_ESP32_UBIRCH_PROTOCOL_C8Y_H
