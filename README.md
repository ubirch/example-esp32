[![ubirch GmbH](files/ubirch_logo.png)](https://ubirch.de)

# How To implement the ubirch protocol on the ESP32
1. [Required packages](#Required-packages)
    1. [ESP32-IDF](#ESP32-IDF)
    1. [ubirch-protocol](#ubirch-protocol)
    1. [ubirch-mbed-msgpack](#ubirch-mbed-msgpack)
    1. [ubirch-mbed-nacl-cm0](#ubirch-mbed-nacl-cm0)
    3. [example project ESP32](#example-project-ESP32)
4. []()
5. []()


## Required packages

The presented ubirch-protocol implementation on the ESP32 platform is 
based on the following prerequisites. So the first steps are to download
and setup the necessary tools and software packages.

### ESP32-IDF

The ESP32 runs on a freeRTOS operating system, customized for the espressif Controllers.
Therefore, the Espressif IoT Development Framework ([ESP-IDF](https://github.com/espressif/esp-idf))
has to be downloaded and set up.

```[bash]
$ git clone git@github.com:espressif/esp-idf.git
```

Use the [guide](https://docs.espressif.com/projects/esp-idf/en/latest/) to install and configure
the ESP-IDF.



### ubirch-protocol

Get the [ubirch-protocol](https://github.com/ubirch/ubirch-protocol) 

```[bash]
$ git clone git@github.com:ubirch/ubirch-protocol.git
```

### ubirch-mbed-msgpack

Get the [ubirch-mbed-msgpack](https://github.com/ubirch/ubirch-mbed-msgpack)

```[bash]
$ git clone https://github.com/ubirch/ubirch-mbed-msgpack.git
```

### ubirch-mbed-nacl-cm0

Get the [ubirch-mbed-nacl-cm0](https://github.com/ubirch/ubirch-mbed-nacl-cm0)

```[bash]
$ git clone https://github.com/ubirch/ubirch-mbed-nacl-cm0.git
```


### example project ESP32

The example project [example-esp32](https://github.com/ubirch/example-esp32) can be used to implement
an application on the ESP32, which uses the ubirch-protocol.

```[bash]
$ git clone https://github.com/ubirch/example-esp32.git
```

## Basic functionality of the example

- generate keys, see [keyHandling.h](main/keyHandling.h) -> createKeys()
- store keys, see [keyHandling.h](main/keyHandling.h) -> storeKeys()
- register keys at the backend, see [keyHandling.h](main/keyHandling.h) -> registerKeys()
- store the previous signature (from the last message), [keyHandling.h](main/keyHandling.h) -> storeSignature() 
- store the public key from the backend, to verify the incoming message replies (currenty hard coded public key)
- create a message in msgpack format, according to ubirch-protocol, see [http.h](main/http.h) -> create_message()
- evaluate the message response, see [http.h](main/http.h) -> checkResponse()

  

## Build your application

To build an application, a customary make file [component.mk](main/component.mk) is required, which
connects the application to the IDF and allows to build the complete firmware and also to use the IDF 
make commands. 

To build the application type:
``` $ make all``` 
or
``` $ make app```

To cleanup the build directory type:
``` $ make clear```

To flash the device, type:
``` $ make flash```

To see the console output, type: 
``` $ make monitor```
or use your prefered serial console.



## Key registration
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

## creating message
After the public key is registered in the backend, the message transmission can take place.

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

// create array[ timestamp, value ])
msgpack_pack_array(pk, 2);
uint64_t ts = getTimeUs();
msgpack_pack_uint64(pk, ts);
uint32_t fake_temp = (esp_random() & 0x0F);
msgpack_pack_int32(pk, (int32_t) (fake_temp));
// finish the protocol
ubirch_protocol_finish(proto, pk);
if (storeSignature(proto->signature, UBIRCH_PROTOCOL_SIGN_SIZE)) {
    ESP_LOGW(TAG, "error storing the signature");
}
// send the message
print_message((const char *) (sbuf->data), (size_t) (sbuf->size));
http_post_task(UHTTP_URL, (const char *) (sbuf->data), sbuf->size);

// clear buffer for next message
msgpack_sbuffer_clear(sbuf);
// free packer and protocol
msgpack_packer_free(pk);
ubirch_protocol_free(proto);
```
 
## evaluate the message response

```C
// create a new unpacker for the receive message
msgpack_unpacker *unpacker = NULL;
unpacker = msgpack_unpacker_new(rcv_buffer_size);
// write the data to the unpacker    
if (msgpack_unpacker_buffer_capacity(unpacker) < evt->data_len) {
    msgpack_unpacker_reserve_buffer(unpacker, evt->data_len);
}
memcpy(msgpack_unpacker_buffer(unpacker), evt->data, evt->data_len);
msgpack_unpacker_buffer_consumed(unpacker, evt->data_len);
// verify the respons
if (!ubirch_protocol_verify(unpacker, esp32_ed25519_verify)) {
    ESP_LOGI(TAG, "check successful");
    parse_payload(unpacker);
} else { // verification failed }

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
                    if (envelope->type == MSGPACK_OBJECT_MAP) {
                        msgpack_object_kv *map = envelope->via.map.ptr;
                        for (uint32_t entries = 0; entries < envelope->via.map.size; map++, entries++) {
                            if (match(map, "i", MSGPACK_OBJECT_POSITIVE_INTEGER)) {
                                // read new interval setting
                                unsigned int value = (unsigned int) (map->val.via.u64);
                                response = value;
                            }
                            //
                            // this is the space, where Parameters from the UI can be added
                            //                   
                        }
                    }
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

