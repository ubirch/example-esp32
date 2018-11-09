[![ubirch GmbH](files/cropped-uBirch_Logo.png)](https://ubirch.de)

# How To implement the ubirch protocol on the ESP32
1. [Required packages](#required-packages)
    1. [ESP32-IDF](#esp32-idf)
    1. [example project ESP32](#example-project-esp32)
    1. [ubirch-protocol](#ubirch-protocol)
    1. [ubirch-mbed-msgpack](#ubirch-mbed-msgpack)
    1. [ubirch-mbed-nacl-cm0](#ubirch-mbed-nacl-cm0)
1. [Build your application](#build-your-application)
    1. [settings.h](#settingsh)
1. [Register your device in the Backend](#register-your-device-in-the-backend)
1. [Basic functionality of the example](#basic-functionality-of-the-example)
    1. [Key registration](#key-registration)
    1. [Message creation](#message-creation)
    1. [Message response evaluation](#message-response-evaluation)


## Required packages

The presented ubirch-protocol implementation on the ESP32 platform is 
based on the following prerequisites. So the first steps are to download
and setup the necessary tools and software packages.

Use this path structure for the packages:
```[bash]
  …
  - /ESP-IDF-DIR
                /ESP32-IDF
  …
  - /PROJECT-DIR
                /example-esp32
                /example-esp32/ubirch-protocol
                /example-esp32/ubirch-mbed-msgpack
                /example-esp32/ubirch-mbed-nacl-cm0
  …
```

### ESP32-IDF

The ESP32 runs on a freeRTOS operating system, customized for the espressif Controllers.
Therefore, the Espressif IoT Development Framework ([ESP-IDF](https://github.com/espressif/esp-idf))
has to be downloaded and set up.

```[bash]
$ git clone https://github.com/espressif/esp-idf.git
```

Use the [guide](https://docs.espressif.com/projects/esp-idf/en/latest/) to install and configure
the ESP-IDF.

### example project ESP32

The example project [example-esp32](https://github.com/ubirch/example-esp32) can be used to implement
an application on the ESP32, which uses the ubirch-protocol.

```bash
$ git clone https://github.com/ubirch/example-esp32.git
```

> Clone the following libraries within `example-esp32`

### ubirch-protocol

Get the [ubirch-protocol](https://github.com/ubirch/ubirch-protocol) 

```bash
$ git clone https://github.com/ubirch/ubirch-protocol.git
```

### ubirch-mbed-msgpack

Get the [ubirch-mbed-msgpack](https://github.com/ubirch/ubirch-mbed-msgpack)

```bash
$ git clone https://github.com/ubirch/ubirch-mbed-msgpack.git
```

### ubirch-mbed-nacl-cm0

Get the [ubirch-mbed-nacl-cm0](https://github.com/ubirch/ubirch-mbed-nacl-cm0)

```bash
$ git clone https://github.com/ubirch/ubirch-mbed-nacl-cm0.git
```

## Build your application

To build an application, a cmake installation is required, which
connects the application to the IDF and allows to build the complete firmware and also to use the IDF 
make commands. 

To prepare your workspace, use the following commands:

```bash
$ mkdir build
$ cd build
$ cmake ..
```

To build the application type:
``` $ make all``` 
or
``` $ make app```

To cleanup the build directory type:
``` $ make clean```

To flash the device, type:
``` $ make app-flash```

To see the console output, type: 
``` $ make monitor```
or use your prefered serial console.

### settings.h

To connect to your wifi, and also to be able to make a http request, create a file ```settings.h``` in the 
 ```/example-esp32/main/``` directory and include the service URLs as well as the wifi settings for your wifi.

```c
// UBIRCH MESSAGE SERVICE
#define UHTTP_PORT 80
#define UHTTP_HOST "unsafe.api.ubirch.demo.ubirch.com"
#define UHTTP_URL "http://unsafe.api.ubirch.demo.ubirch.com/api/avatarService/v1/device/update/mpack"
// UBIRCH KEY SERVICE
#define UKEY_SERVICE_PORT 80
#define UKEY_SERVICE_HOST "unsafe.key.demo.ubirch.com"
#define UKEY_SERVICE_URL  "http://unsafe.key.demo.ubirch.com/api/keyService/v1/pubkey/mpack"

#define EXAMPLE_WIFI_SSID "YOUR_WIFI_SSID"
#define EXAMPLE_WIFI_PASS "YOUR_WIFI_PASSWORD"
#define UBIRCH_AUTH "ubirch-dev::<YOURTOKEN>::<SIGNATURE>"
```

## Register your device in the Backend

To register your device ath the backend, follow the [ubirch Cloud Services Guideline](https://developer.ubirch.com/cloud-services.html) 
Therefore the hardware device UUID of the device is needed, which is printed out on the cerial console, 
when the device is reseted, see below:

```[bash]
...
hello this is Ubirch protocol on ESP32 wifi example 

I (209) UUID: FFFFFFFF-FFFF-1223-3445-566778899AAB

I (212) wifi: wifi driver task:.......
...
```
Copy the UUID and register the device.

## Basic functionality of the example

- generate keys, see [createKeys()](https://github.com/ubirch/example-esp32/blob/master/main/keyHandling.h#L40)
- store keys, see [storeKeys()](https://github.com/ubirch/example-esp32/blob/master/main/keyHandling.h#L64)
- register keys at the backend, see [registerKeys()](https://github.com/ubirch/example-esp32/blob/master/main/keyHandling.h#L89)
- store the previous signature (from the last message), [storeSignature()](https://github.com/ubirch/example-esp32/blob/master/main/keyHandling.h#L84)
- store the public key from the backend, to verify the incoming message replies ([currenty hard coded public key](https://github.com/ubirch/example-esp32/blob/master/main/keyHandling.c#L49-L54))
- create a message in msgpack format, according to ubirch-protocol, see [create_message()](https://github.com/ubirch/example-esp32/blob/master/main/ubirch-proto-http.h#L77)
- make a http post request, see [http_post_task()](https://github.com/ubirch/example-esp32/blob/master/main/ubirch-proto-http.h#L49)
- evaluate the message response, see [checkResponse()](https://github.com/ubirch/example-esp32/blob/master/main/ubirch-proto-http.h#L54)
- react to the UI message response parameter "i" to turn on the blue LED, if the value is above 1000, see [app_main()](https://github.com/ubirch/example-esp32/blob/master/main/main.c#L67-L69) 


### Key registration
The public key of the ESP32 aplication has to be provided to the backend. 

The example already includes the functionality in [registerKeys()](main/keyHandling.h)

```C
// create buffer, protocol and packer
msgpack_sbuffer *sbuf = msgpack_sbuffer_new();
ubirch_protocol *proto = ubirch_protocol_new(proto_signed, UBIRCH_PROTOCOL_TYPE_REG,
                                             sbuf, msgpack_sbuffer_write, ed25519_sign, UUID);
msgpack_packer *pk = msgpack_packer_new(proto, ubirch_protocol_write);
// start the ubirch protocol message
ubirch_protocol_start(proto, pk);
// create key registration info
ubirch_key_info info = {};                              
info.algorithm = (char *)(UBIRCH_KEX_ALG_ECC_ED25519);  
info.created = time(NULL);                                  // current time of the system
memcpy(info.hwDeviceId, UUID, sizeof(UUID));                // 16 Byte unique hardware device ID
memcpy(info.pubKey, ed25519_public_key, sizeof(ed25519_public_key));    // the public key
info.validNotAfter = time(NULL) + 31536000;                 // time until the key will be valid (now + 1 year)
info.validNotBefore = time(NULL);                           // time from when the key will be valid (now)
// pack the key registration msgpack
msgpack_pack_key_register(pk, &info);
// finish the ubirch protocol message
ubirch_protocol_finish(proto, pk);
// send the data
http_post_task(UKEY_SERVICE_URL, sbuf->data, sbuf->size);
// free allocated ressources
msgpack_packer_free(pk);
ubirch_protocol_free(proto);
msgpack_sbuffer_free(sbuf);
```

### Message creation

After the public key is registered in the backend, messages for sensor values can be created and transmitted, 
like in the example from [create_message()](main/ubirch-proto-http.h)

```C
// create buffer, writer, ubirch protocol context and packer
msgpack_sbuffer *sbuf = msgpack_sbuffer_new();
ubirch_protocol *proto = ubirch_protocol_new(proto_chained, MSGPACK_MSG_UBIRCH,
                                             sbuf, msgpack_sbuffer_write, esp32_ed25519_sign, UUID);
msgpack_packer *pk = msgpack_packer_new(proto, ubirch_protocol_write);
// load the old signature and copy it to the protocol
unsigned char old_signature[UBIRCH_PROTOCOL_SIGN_SIZE] = {};
if (loadSignature(old_signature)) {
    ESP_LOGW(TAG, "error loading the old signature");
}
memcpy(proto->signature, old_signature, UBIRCH_PROTOCOL_SIGN_SIZE);
// start the protocol
ubirch_protocol_start(proto, pk);
//
// PAYLOAD SECTION
// add or modify the data here
//
// create array[ timestamp, value ])
msgpack_pack_array(pk, 1);
uint64_t ts = getTimeUs();
msgpack_pack_uint64(pk, ts);
uint32_t fake_temp = (esp_random() & 0x0F);
msgpack_pack_int32(pk, (int32_t) (fake_temp));
//
// END OF PAYLOAD
//
// finish the protocol
ubirch_protocol_finish(proto, pk);
if (storeSignature(proto->signature, UBIRCH_PROTOCOL_SIGN_SIZE)) {
    ESP_LOGW(TAG, "error storing the signature");
}
// send the message
http_post_task(UHTTP_URL, (const char *) (sbuf->data), sbuf->size);

// free allocated ressources
msgpack_packer_free(pk);
ubirch_protocol_free(proto);
msgpack_sbuffer_free(sbuf);
```
 
## Message response evaluation

The message response evaluation is performed in several steps, by several functions. 
The details are discribed below in consecutive steps.
 
- the unpacker is a global array pointer, where the data is stored, see [(code)](https://github.com/ubirch/example-esp32/blob/master/main/ubirch-proto-http.c#L63)
```c
msgpack_unpacker *unpacker = NULL;
```
- generate a new unpacker at the beginning of every transmission, see [(code)](https://github.com/ubirch/example-esp32/blob/master/main/ubirch-proto-http.c#L117)
```c
unpacker = msgpack_unpacker_new(rcv_buffer_size);
```
- store the incoming receive message data into the buffer, see [(code)](https://github.com/ubirch/example-esp32/blob/master/main/ubirch-proto-http.c#L97-L101)
```c
// write the data to the unpacker    
if (msgpack_unpacker_buffer_capacity(unpacker) < evt->data_len) {
    msgpack_unpacker_reserve_buffer(unpacker, evt->data_len);
}
memcpy(msgpack_unpacker_buffer(unpacker), evt->data, evt->data_len);
msgpack_unpacker_buffer_consumed(unpacker, evt->data_len);
```
- if the http post was successful -> status < 300, verify the response, see [(code)](https://github.com/ubirch/example-esp32/blob/master/main/ubirch-proto-http.c#L151-L156)
```c
if (!ubirch_protocol_verify(unpacker, esp32_ed25519_verify)) {
    ESP_LOGI(TAG, "check successful");
    parse_payload(unpacker);
} else { // verification failed }
```
- if the verification was successful, parse the payload, , see [(code)](https://github.com/ubirch/example-esp32/blob/master/main/ubirch-proto-http.c#L162-L201)
```c
// new unpacked result buffer
msgpack_unpacked result;
msgpack_unpacked_init(&result);
// unpack into result buffer and look for ARRAY
if (msgpack_unpacker_next(unpacker, &result) && result.data.type == MSGPACK_OBJECT_ARRAY) {
    // redirect the result to the envelope
    msgpack_object *envelope = result.data.via.array.ptr;
    int p_version = 0;
    // get the envelope version
    if (envelope->type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
        p_version = (int) envelope->via.u64;
    }
    // get the backend UUID
    if ((++envelope)->type == MSGPACK_OBJECT_RAW) {
        ESP_LOGI(TAG, "UUID: ");
        print_message(envelope->via.raw.ptr, envelope->via.raw.size);
    }
    // only continue if the envelope version and variant match
    if (p_version == proto_chained) {
        // previous message signature (from our request message)
        unsigned char prev_signature[UBIRCH_PROTOCOL_SIGN_SIZE];
        if (loadSignature(prev_signature)) {
        }
        // compare the previous signature to the received one
        bool prevSignatureMatches = false;
        if ((++envelope)->type == MSGPACK_OBJECT_RAW) {
            if (envelope->via.raw.size == crypto_sign_BYTES) {
                prevSignatureMatches = !memcmp(prev_signature, envelope->via.raw.ptr, UBIRCH_PROTOCOL_SIGN_SIZE);
            }
        }
        // only continue, if the signatures match
        if (prevSignatureMatches && (++envelope)->type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            switch ((unsigned int) envelope->via.u64) {
                case MSGPACK_MSG_REPLY:
```

- compare the signatures and if they match, continue with the payload, see [(code)](https://github.com/ubirch/example-esp32/blob/master/main/ubirch-proto-http.c#L229-L251)
```c
                    if (envelope->type == MSGPACK_OBJECT_MAP) {
                        msgpack_object_kv *map = envelope->via.map.ptr;
                        for (uint32_t entries = 0; entries < envelope->via.map.size; map++, entries++) {
                            //
                            // UI PARAMETER SECTION
                            // ad or modify the used parameters here
                            //
                            if (match(map, "i", MSGPACK_OBJECT_POSITIVE_INTEGER)) {
                                // read new interval setting
                                unsigned int value = (unsigned int) (map->val.via.u64);
                                ESP_LOGI(TAG, "i = %d ", value);
                                response = value;
                            } 
                            //
                            // END OF PARAMETER SECTION
                            //
                        }
                    }
```
- delete all temporary buffers, see [(code)](https://github.com/ubirch/example-esp32/blob/master/main/ubirch-proto-http.c#L204-L221)
```c
                    break;
                case UBIRCH_PROTOCOL_TYPE_HSK:
                    //TODO handshake reply evaluation
                    break;
                default:
                    break;
            }
        }
    }
}
msgpack_unpacked_destroy(&result);
```
- delete the unpacker, see [(code)](https://github.com/ubirch/example-esp32/blob/master/main/ubirch-proto-http.c#L145)
```c
msgpack_unpacker_free(unpacker);

```
