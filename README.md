[![ubirch GmbH](files/cropped-uBirch_Logo.png)](https://ubirch.de)

# How To implement the ubirch protocol on the ESP32
1. [Required packages](#required-packages)
    1. [xtensa-toolchain](#xtensa-toolchain)
    1. [ESP32-IDF](#esp32-idf)
    1. [example project ESP32](#example-project-esp32)
        1. [The submodules](#the-submodules)
    1. [ubirch-protocol](#ubirch-protocol)
    1. [ubirch-mbed-msgpack](#ubirch-mbed-msgpack)
    1. [ubirch-mbed-nacl-cm0](#ubirch-mbed-nacl-cm0)
1. [Build your application](#build-your-application)
1. [Register your device in the Backend](#register-your-device-in-the-backend)
1. [Basic functionality of the example](#basic-functionality-of-the-example)
    1. [Key registration](#key-registration)
    1. [Message creation](#message-creation)
    1. [Message response evaluation](#message-response-evaluation)

## Required packages

The presented ubirch-protocol implementation on the ESP32 platform is
based on the following prerequisites. So the first steps are to download
and setup the necessary tools and software packages.


### xtensa toolchain

The xtensa toolchain provides the necessary compiler to build the firmware and applcation for the ESP32.
Follow the [linux-guide](https://dl.espressif.com/doc/esp-idf/latest/get-started/linux-setup.html)
to install the toolchain on your linux machine.

### ESP32-IDF

The ESP32 runs on a freeRTOS operating system, customized for the espressif Controllers.
Therefore, the Espressif IoT Development Framework ([ESP-IDF](https://github.com/espressif/esp-idf))
has to be downloaded and set up.

```[bash]
git clone https://github.com/espressif/esp-idf.git
```

Use the [guide](https://docs.espressif.com/projects/esp-idf/en/latest/) to install and configure
the ESP-IDF.

### example project ESP32

The example project [example-esp32](https://github.com/ubirch/example-esp32) can be used to implement
an application on the ESP32, which uses the ubirch-protocol.

```bash
git clone --recursive https://github.com/ubirch/example-esp32.git
```

> If you have already cloned the example, but do not have the submodules yet,
you can get the submodules with the following commands:

```bash
cd example-esp && \
git submodule update --init --recursive
```

Now the structure on your machine should look like this:
```bash

|-- ESP-IDF
|..
|-- xtensa-toolchain
|..
|-- example-esp
        |-- components
                |-- arduino-esp
                |-- ubirch-esp32-api-http
                |-- ubirch-esp32-console
                |-- ubirch-esp32-networking
                |-- ubirch-esp32-ota
                |-- ubirch-esp32-storage
                |-- ubirch-mbed-msgpack
                |-- ubirch-mbed-nacl-cm0
                |-- ubirch-protocol
        |..
```

#### The submodules

For the documentation of the herein used submodules, go to the repositories, or the README.md files inside the modules.

This list provides the links to the submodule repositories:

- [ubirch-mbed-msgpack](https://github.com/ubirch/ubirch-mbed-msgpack)
- [ubirch-mbed-nacl-cm0](https://github.com/ubirch/ubirch-mbed-nacl-cm0)
- [ubirch-protocol](https://github.com/ubirch/ubirch-protocol.git)
- [arduino-esp32](https://github.com/ubirch/arduino-esp32)
- [ubirch-esp32-networking](https://github.com/ubirch/ubirch-esp32-networking)
- [ubirch-esp32-console](https://github.com/ubirch/ubirch-esp32-console)
- [ubirch-esp32-storage](https://github.com/ubirch/ubirch-esp32-storage)
- [ubirch-esp32-api-http](https://github.com/ubirch/ubirch-esp32-api-http)
- [ubirch-esp32-ota](https://github.com/ubirch/ubirch-esp32-ota)

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

> Before flashing the first time, please run `make menuconfig` once and select *Partition Table" and set it to
> `Partition Table (Factory app, two OTA definitions)`. Exit this sub menu and select *Serial Flasher config*
> and set the `Flash size (4 MB)`. <br/>
> **Make sure to *Save* the settings and *Exit* the configuration.**

To build the application type:
``` $ make all```
or
``` $ make app```

To cleanup the build directory type:
``` $ make clean```

To flash the device, type:
``` $ make app-flash```

## Serial Interface

The Serial interface for the ESP32 is managed via a a specific interaface chip on your ESP32 board,
according to the [Establish Serial Connection with ESP32 guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/establish-serial-connection.html#establish-serial-connection-with-esp32)

### Tools for Serial Connection

- `$IDF_PATH/tools/idf.py monitor`
- `miniterm.py` is a tool from the [pyserial](https://github.com/pyserial/pyserial) package.
    -  to use miniterm.py, open your prefered terminal and enter: `miniterm.py /dev/ttyUSB0 115200 --raw`
        - where `/dev/ttyUSB0` is the COM port and `115200` is the baudrate. Both might have to be adjusted. the `--raw` give you the raw output, without the special control characters.


### Console

The console is a [submodule](components/ubirch-esp32-console) of the example,
which provides a way to locally communicate with the device, via serial interface.
Check the [README](components/ubirch-esp32-console/README.md) for more details.

#### Enter Console mode

To enter the console mode, press `Ctrl + c` on your keyboard, while you are in a terminal connected to the device.


### Erase the device memory

To erase the complete chip memory, go to the directory
`esp-idf/components/esptool_py/esptool` and use the command

```bash
$IDF_PATH/components/esptool_py/esptool/esptool.py erase_flash
```

> If you erased the complete chip memory, the whole firmware, including
the bootloader has to be flashed onto the chip, otherwise the esp32
device cannot run. properly.

## Register your device in the Backend

To register your device ath the backend, follow the [ubirch Cloud Services Guideline](https://developer.ubirch.com/cloud-services.html)
Therefore the hardware device ID of the device is needed, which is printed out on the [console](#console) with the command `status`.

The corresponding output may look like this:

```[bash]
UBIRCH device status:
Hardware-Device-ID: XXXXXXXX-XXXX-1223-3445-566778899AAB
Public key: 89088729D730C1DBE9E3392B85ABF562EBC51580A5DAC7B819DC7D368EE3CCF4
Wifi SSID : YOUR-WIFI-SSID
Current time: 10.12.2018 14:29:49
```
Copy the UUID and register the device.

## Basic functionality of the example

- generate keys, see [create_keys()](https://github.com/ubirch/example-esp32/blob/master/main/key_handling.h#L40)
- store keys, see [store_keys()](https://github.com/ubirch/example-esp32/blob/master/main/key_handling.h#L64)
- register keys at the backend, see [register_keys()](https://github.com/ubirch/example-esp32/blob/master/main/key_handling.h#L89)
- store the previous signature (from the last message), [store_signature()](https://github.com/ubirch/example-esp32/blob/master/main/key_handling.h#L84)
- store the public key from the backend, to verify the incoming message replies ([currenty hard coded public key](https://github.com/ubirch/example-esp32/blob/master/main/key_handling.c#L49-L54))
- create a message in msgpack format, according to ubirch-protocol, see [create_message()](https://github.com/ubirch/example-esp32/blob/master/main/ubirch-proto-http.h#L80)
- make a http post request, see [http_post_task()](https://github.com/ubirch/example-esp32/blob/master/main/ubirch-proto-http.h#L49)
- evaluate the message response, see [check_response()](https://github.com/ubirch/example-esp32/blob/master/main/ubirch-proto-http.h#L54)
- react to the UI message response parameter "i" to turn on the blue LED, if the value is above 1000, see [app_main()](https://github.com/ubirch/example-esp32/blob/master/main/main.c#L67-L69) 


### Key registration
The public key of the ESP32 aplication has to be provided to the backend. 

The example already includes the functionality in [register_keys()](https://github.com/ubirch/example-esp32/blob/master/main/key_handling.c#L215-L242)

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
like in the example from [create_message()](https://github.com/ubirch/example-esp32/blob/master/main/ubirch-proto-http.c#L266-L309)

```C
// create buffer, writer, ubirch protocol context and packer
msgpack_sbuffer *sbuf = msgpack_sbuffer_new();
ubirch_protocol *proto = ubirch_protocol_new(proto_chained, MSGPACK_MSG_UBIRCH,
                                             sbuf, msgpack_sbuffer_write, esp32_ed25519_sign, UUID);
msgpack_packer *pk = msgpack_packer_new(proto, ubirch_protocol_write);
// load the old signature and copy it to the protocol
unsigned char old_signature[UBIRCH_PROTOCOL_SIGN_SIZE] = {};
if (load_signature(old_signature)) {
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
uint64_t ts = get_time_us();
msgpack_pack_uint64(pk, ts);
uint32_t fake_temp = (esp_random() & 0x0F);
msgpack_pack_int32(pk, (int32_t) (fake_temp));
//
// END OF PAYLOAD
//
// finish the protocol
ubirch_protocol_finish(proto, pk);
if (store_signature(proto->signature, UBIRCH_PROTOCOL_SIGN_SIZE)) {
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
        if (load_signature(prev_signature)) {
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
