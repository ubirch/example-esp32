[![ubirch GmbH](files/ubirch_logo.png)](https://ubirch.de)

# How To implement the ubirch protocol on the ESP32

The presented ubirch-protocol implementation on the ESP32 platform is 
based on the following prerequisites. So the first steps are to download
and setup the necessary tools and software packages.

## ESP32-IDF

The ESP32 runs on a freeRTOS operating system, customized for the espressif Controllers.
Therefore, the Espressif IoT Development Framework ([ESP-IDF]((https://github.com/espressif/esp-idf)))
has to be downloaded and set up.

```[bash]
$ git clone git@github.com:espressif/esp-idf.git
```

Use the [guide](https://docs.espressif.com/projects/esp-idf/en/latest/) to install and configure
the ESP-IDF.



## ubirch-protocol

Get the [ubirch-protocol](https://github.com/ubirch/ubirch-protocol) 

```[bash]
$ git clone git@github.com:ubirch/ubirch-protocol.git
```

## ubirch-mbed-msgpack

Get the [ubirch-mbed-msgpack](https://github.com/ubirch/ubirch-mbed-msgpack)

```[bash]
$ git clone https://github.com/ubirch/ubirch-mbed-msgpack.git
```

## ubirch-mbed-nacl-cm0

Get the [ubirch-mbed-nacl-cm0](https://github.com/ubirch/ubirch-mbed-nacl-cm0)

```[bash]
$ git clone https://github.com/ubirch/ubirch-mbed-nacl-cm0.git
```


## Download example project ESP32

[TODO](also to do)

- the application has to generate and store keys
- the application has to store the previous signature (from the last message)
- the application has to store the public key from the backend, to verify the incoming message replies


## Download  


## Key registration
The public key of the ESP32 aplication has to be provided to the backend. 
To do so, the 
The example already includes the functionality in [registerKeys()](main/keyHandling.h)

```c
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
 

