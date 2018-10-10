#ifndef _HTTP_H_
#define _HTTP_H_

//#define DEMO

#ifdef DEMO
#define UHTTP_PORT 80
#define UHTTP_HOST "unsafe.api.ubirch.demo.ubirch.com"
#define UHTTP_URL "http://unsafe.api.ubirch.demo.ubirch.com/api/avatarService/v1/device/update/mpack"
#define UKEY_SERVICE_PORT 80
#define UKEY_SERVICE_HOST "unsafe.key.demo.ubirch.com"
#define UKEY_SERVICE_URL  "http://unsafe.key.demo.ubirch.com/api/keyService/v1/pubkey/mpack"
#else
#define UHTTP_PORT 80
#define UHTTP_HOST "unsafe.api.ubirch.dev.ubirch.com"
#define UHTTP_URL "http://unsafe.api.ubirch.dev.ubirch.com/api/avatarService/v1/device/update/mpack"
#define UKEY_SERVICE_PORT 80
#define UKEY_SERVICE_HOST "unsafe.key.dev.ubirch.com"
#define UKEY_SERVICE_URL  "http://unsafe.key.dev.ubirch.com/api/keyService/v1/pubkey/mpack"
#endif



#define MSGPACK_MSG_REPLY 85

void http_init();

void http_post_task(const char *url, const char *data, const size_t length);

void check_response();

void parse_payload(msgpack_unpacker *unpacker);

void parseMeasurementReply(msgpack_object *envelope);

bool match(const msgpack_object_kv *map, const char *key, const int type);



#endif /*_HTTP_H_*/

