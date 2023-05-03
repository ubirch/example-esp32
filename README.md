[![ubirch GmbH](files/cropped-uBirch_Logo.png)](https://ubirch.de)

# How To implement the ubirch protocol on the ESP32

1. [Required packages](#required-packages)
    1. [ESP32-IDF and xtensa toolchain](#esp32-idf-and-xtensa-toolchain)
    1. [example project ESP32](#example-project-esp32)
        1. [The submodules](#the-submodules)
1. [Pre-build configuration](#pre-build-configuration)
1. [Build your application](#build-your-application)
1. [Device configuration](#device-configuration)
    1. [Configuration of a single ubirch ID with generated uuid](#configuration-of-a-single-ubirch-id-with-generated-uuid)
    1. [Configuration of single ubirch ID with pre-generated uuid](#configuration-of-single-ubirch-id-with-pre-generated-uuid)
    1. [Configuration of multiple identities with nvs-flash tools](#configuration-of-multiple-identities-with-nvs-flash-tools)
1. [Serial Interface](#serial-interface)
    1. [Tools for Serial Connection](#tools-for-serial-connection)
    1. [Console](#console)
        1. [Enter Console mode](#enter-console-mode)
    1. [Erase the device memory](#erase-the-device-memory)
1. [Register your device in the Backend](#register-your-device-in-the-backend)
1. [Basic functionality of the example](#basic-functionality-of-the-example)
1. [Ubirch specific functionality](#ubirch-specific-functionality)
1. [Build and run tests](#build-and-run-tests)

## Required packages

The presented ubirch-protocol implementation on the ESP32 platform is
based on the following prerequisites. So the first steps are to download
and setup the necessary tools and software packages.

### ESP32-IDF and xtensa toolchain

The ESP32 runs on a freeRTOS operating system, customized for the espressif Controllers.
Therefore, the Espressif IoT Development Framework ([ESP-IDF](https://github.com/espressif/esp-idf))
has to be downloaded and set up.

```[bash]
git clone -b v4.3 --recursive https://github.com/espressif/esp-idf.git
```

Make sure you have esp-idf release `v4.3`. The installation of the xtensa toolchain is done
via the `install.{sh, bat}` script and the configuration of your environment by `source`ing
the `export.{sh, bat, ps1, fish}` within the esp-idf.  For detailed instructions follow the
[guide](https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/get-started/index.html#step-2-get-esp-idf).

> **The example was tested on esp-idf release `v4.2` and `v4.3`.**

### example project ESP32

The example project [example-esp32](https://github.com/ubirch/example-esp32) can be used to implement
an application on the ESP32, which uses the ubirch-protocol.

```bash
git clone --recursive https://github.com/ubirch/example-esp32.git
```

> If you have already cloned the example, but do not have the submodules yet,
you can get the submodules with the following commands:

```bash
cd example-esp32 && \
git submodule update --init --recursive
```

Now the structure on your machine should look like this:
```bash

|-- ESP-IDF
|..
|-- xtensa-toolchain
|..
|-- example-esp32
        |-- components
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

- [ubirch-msgpack](https://github.com/ubirch/msgpack-c)
- [ubirch-mbed-nacl-cm0](https://github.com/ubirch/ubirch-mbed-nacl-cm0)
- [ubirch-protocol](https://github.com/ubirch/ubirch-protocol.git)
- [ubirch-esp32-networking](https://github.com/ubirch/ubirch-esp32-networking)
- [ubirch-esp32-console](https://github.com/ubirch/ubirch-esp32-console)
- [ubirch-esp32-storage](https://github.com/ubirch/ubirch-esp32-storage)
- [ubirch-esp32-api-http](https://github.com/ubirch/ubirch-esp32-api-http)
- [ubirch-esp32-ota](https://github.com/ubirch/ubirch-esp32-ota)


## Pre-build configuration

The URLs for the data, keys and fimrware updates can be configured by running from the `build` directory,
or if you are working with Clion, from the `cmake-build-debug` directory:
```bash
make menuconfig
```
Go to the Category `UBIRCH` to setup the URL for the firmware update
and go to `UBIRCH Application` to setup the URL fot the ubirch-protocol data,
the key server URL and to adjust the measuring interval.


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
`$ make all`
or
`$ make app`

To cleanup the build directory type:
`$ make clean`

To make sure you start from a clean device type:
`$ esptool.py erase_flash`

To flash the device, type:
`$ make app-flash`


## Device configuration

### Configuration of a single ubirch ID with generated uuid

After [flashing the firmware](#build-your-application) you can [enter Console mode](#enter-console-mode)
and use the `status` command to get your devices uuid.
With this uuid you can [register your device in the Backend](#register-your-device-in-the-backend)
and get the device-password from ubirch-console.

To set the password run
```[bash]
update_password <password>
```

To set the default backend key run
```[bash]
update_backendkey
```

Join the wifi with
```[bash]
join YOUR-WIFI-SSID YOUR-WIFI-PWD
```

and restart with
```[bash]
restart
```

When the device is connected to the internet it can get a time-update and creates a new key-pair
which is then registered in the ubirch backend.


### Configuration of single ubirch ID with pre-generated uuid

Use `$ uuidgen` to generate a new uuid and [register your device with this uuid in the Backend](#register-your-device-in-the-backend).
Create a json-file (e.g. `my_device_config.json`) including the following json object with the information from your registered device:
```json
{
  "short_name": "default_id",
  "uuid": "<uuid>",
  "password": "<password>"
}
```

Note that the `short_name` should be `default_id` if you are using a single ID in this example.

Create the nvs-flash-partition description csv-file and binary-file (for more details compare
[the esp-idf documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_partition_gen.html)) by running:
```bash
$ python create_ubirch_devices.py my_device_config.json
$ python $IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py generate my_device_config.csv my_device_config.bin 0x3000
```

Flash the partition to your device:
```bash
$ parttool.py write_partition --partition-name=nvs --input my_device_config.bin
```

[Enter Console mode](#enter-console-mode) to connect to wifi:
```[bash]
join YOUR-WIFI-SSID YOUR-WIFI-PWD
```

When the device is connected to the internet it can get a time-update and creates a new key-pair
which is then registered in the ubirch backend.

To backup the whole configuration including the key-pair:
```bash
$ parttool.py read_partition --partition-name=nvs --output my_device_config_backup.bin
```

### Configuration of multiple identities with nvs-flash tools

To use multiple ubirch IDs on your device enter menuconfig via `$ make menuconfig`,
navigate to `Ubirch Application` and set `Enable multiple ids`. 

Follow the same steps as in [the previous section](#configuration-of-single-ubirch-id-with-pre-generated-uuid)
but register multiple devices and put a list of them in the json-file:
```json
[
  {
    "short_name": "foo",
    "uuid": "<uuid-1>",
    "password": "<password-1>"
  },
  {
    "short_name": "bar",
    "uuid": "<uuid-2>",
    "password": "<password-2>"
  }
]
```

Note that the `short_name`s are used to load the IDs from memory, see [here](https://github.com/ubirch/example-esp32/blob/master/main/main.c#L96).

The number of IDs that can be used at once depend on the partition size.
Each ID needs about 230 byte of memory (including nvs overhead), the backend key
needs 64 byte (including nvs overhead) and the wifi credentials need space depending on SSID and password length.

## Serial Interface

The Serial interface for the ESP32 is managed via a a specific interaface chip on your ESP32 board,
according to the [Establish Serial Connection with ESP32 guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/establish-serial-connection.html#establish-serial-connection-with-esp32)

### Tools for Serial Connection

- `$IDF_PATH/tools/idf.py monitor`
- `miniterm.py` is a tool from the [pyserial](https://github.com/pyserial/pyserial) package.
    -  to use miniterm.py, open your prefered terminal and enter: `miniterm.py /dev/ttyUSB0 115200 --raw`
        - where `/dev/ttyUSB0` is the COM port and `115200` is the baudrate. Both might have to be adjusted. the `--raw` give you the raw output, without the special control characters.

### Console

The console is a [component](https://github.com/ubirch/example-esp32/tree/master/components) of the example,
which provides a way to locally communicate with the device, via serial interface.
Check the [README](https://github.com/ubirch/ubirch-esp32-console/blob/master/README.md) for more details.

#### Enter Console mode

To enter the console mode, press `Ctrl + c` or `Ctrl + u` on your keyboard, while you are in a terminal connected to the device.


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

To register your device at the [demo-backend](https://ubirch.demo.ubirch.com/), follow the [ubirch Cloud Services Guideline](https://developer.ubirch.com/cloud-services.html).
Therefore the hardware device ID of the device is needed, which is printed out on the [console](#console) with the command `status`.

Before you can register the device, you need a connection to the internet.
If you are using wifi, go to the [console](#console)  and `join` a Wifi with:

```[bash]
join YOUR-WIFI-SSID YOUR-WIFI-PWD
```

The corresponding output may look like this:

```[bash]
UBIRCH device status:
Hardware-Device-ID: XXXXXXXX-XXXX-1223-3445-566778899AAB
Public key: 89088729D730C1DBE9E3392B85ABF562EBC51580A5DAC7B819DC7D368EE3CCF4
Backend public key: 74BIrQbAKFrwF3AJOBgwxGzsAl0B2GCF51pPAEHC5pA=
Wifi SSID : YOUR-WIFI-SSID
Current time: 28.07.2021 13:19:49
```
Copy the UUID and register the device in the [Ubirch Console Web Interface](https://console.prod.ubirch.com/).

> After the registration, copy the `password` from the `apiConfig` and
 now go back to the [Configuration](#configuration) and enter your password string into `ubirch authentication string`. 

**Then recompile and run the code.**


## Basic functionality of the example

- at startup, the system is initialized, see [init_system()](https://github.com/ubirch/example-esp32/blob/master/main/main.c#L138-L164)
- try to connect to the wifi, if stored wifi settings are available, see [connecting to wifi](https://github.com/ubirch/example-esp32/blob/master/main/main.c#L181-L199)
- create the following tasks, which are afterwards handled by the system:
    - [**enter_console_task**](https://github.com/ubirch/example-esp32/blob/master/main/main.c#L201-L217),
    allows you to enter the console. For details about the console, please refer to the [repository](https://github.com/ubirch/ubirch-esp32-console) and [README](https://github.com/ubirch/ubirch-esp32-console/blob/master/README.md)
    - [**update_time_task**](https://github.com/ubirch/example-esp32/blob/master/main/main.c#L185-L195),
    updates the time via sntp
    - [**ubirch_ota_task**](https://github.com/ubirch/ubirch-esp32-ota/blob/master/ubirch_ota_task.c#L38-L56),
    checks if firmware updates are availabe and performs the updates. For more details, please refer to the [repository](https://github.com/ubirch/ubirch-esp32-ota) and the [README](https://github.com/ubirch/ubirch-esp32-ota/blob/master/README.md)
    - [**main_task**](https://github.com/ubirch/example-esp32/blob/master/main/main.c#L61-L178),
    performs the main functionality of the application.

## Ubirch specific functionality

- generate keys, see [create_keys()](https://github.com/ubirch/ubirch-esp32-api-http/blob/master/keys.h#L50)
- register keys at the backend, see [register_keys()](https://github.com/ubirch/ubirch-esp32-api-http/blob/master/keys.h#L58)
- store the previous signature (from the last message), [ubirch_previous_signature_set()](https://github.com/ubirch/ubirch-esp32-key-storage/blob/master/id_handling.h#L199)
- store the public key from the backend, to verify the incoming message replies (see console command `update_backendkey` [here](https://github.com/ubirch/ubirch-esp32-console#command-overview)) 
- create a message in msgpack format, according to ubirch-protocol, see [ubirch_message()](https://github.com/ubirch/ubirch-esp32-api-http/blob/master/message.h#L44)
- make a http post request, see [ubirch_send()](https://github.com/ubirch/ubirch-esp32-api-http/blob/master/ubirch_api.h#L58)
- evaluate the message response, see [ubirch_parse_backend_response()](https://github.com/ubirch/ubirch-esp32-api-http/blob/master/response.h#L73)

## Build and run tests

To build and run the test runner, that controls tests that are implemented in the components, you need to have the esp-idf as well as the xtensa toolchain installed as described above.
Similar to the [build](#build) section run the following commands to prepare the build (note that you need to change to the `test` directory in the project root):

```bash
$ cd test
$ mkdir build
$ cd build
$ cmake ..
```

Then run ``` $ make``` and ``` $ make flash``` to build and flash your device. To get the output of the tests use the serial interface as described above.
The testrunner will immediately run the tests after flashing, to get all the outputs start your serial monitor right after flashing, e.g.

```bash
make -j flash && miniterm.py /dev/ttyUSB0 115200 --raw
```

You can re-run single tests by using the interactive test menu which is started right after running the tests.
