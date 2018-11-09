# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# check and set the IDF_PATH
message(STATUS "$ENV{IDF_PATH}")
if (NOT DEFINED ENV{IDF_PATH})
    if (EXISTS "$ENV{HOME}/.idf")
        file(STRINGS "$ENV{HOME}/.idf" IDF_PATH)
        set(ENV{IDF_PATH} ${IDF_PATH})
    else ()
        message(FATAL_ERROR "ESP-IDF: set $ESP_IDF environment variable or write path to file $ENV{HOME}/.idf")
    endif ()
else ()
    set(IDF_PATH $ENV{IDF_PATH})
endif ()
message(STATUS "IDF_PATH: ${IDF_PATH}")

# add some necessary defnitions
add_definitions(-DESP_PLATFORM -DRANDOMBYTES_DEFAULT_IMPLEMENTATION)

# set the components, EXCLUDING libsodium, because of the conflict with ubirch-mbed-nacl-cm0
set(COMPONENTS soc;log;heap;freertos;vfs;newlib;esp_ringbuf;driver;esp_event;ethernet;lwip;tcpip_adapter;partition_table;app_update;spi_flash;mbedtls;micro-ecc;bootloader_support;nvs_flash;pthread;smartconfig_ack;wpa_supplicant;xtensa-debug-module;esp32;cxx;app_trace;asio;jsmn;aws_iot;bootloader;bt;coap;console;nghttp;esp-tls;esp_adc_cal;tcp_transport;esp_http_client;esp_http_server;esp_https_ota;esptool_py;expat;wear_levelling;sdmmc;fatfs;freemodbus;idf_test;json;mdns;mqtt;openssl;protobuf-c;protocomm;spiffs;ulp;wifi_provisioning)

#set(CMAKE_VERBOSE_MAKEFILE 1)

# check that necessary sub-projects are available
# msgpack implementation
if (NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/ubirch-mbed-msgpack)
    message(FATAL_ERROR "required library ubirch-mbed-msgpack missing, checkout from github:\n"
            "$ git clone https://github.com/ubirch/ubirch-mbed-msgpack"
            )
endif ()

set(MSGPACK_SRCS
        ${CMAKE_CURRENT_LIST_DIR}/ubirch-mbed-msgpack/objectc.c
        ${CMAKE_CURRENT_LIST_DIR}/ubirch-mbed-msgpack/unpack.c
        ${CMAKE_CURRENT_LIST_DIR}/ubirch-mbed-msgpack/version.c
        ${CMAKE_CURRENT_LIST_DIR}/ubirch-mbed-msgpack/vrefbuffer.c
        ${CMAKE_CURRENT_LIST_DIR}/ubirch-mbed-msgpack/zone.c)

# ED25519 implementation
if (NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/ubirch-mbed-nacl-cm0)
    message(FATAL_ERROR "required library ubirch-mbed-nacl-cm0 missing, checkout from github:\n"
            "$ git clone https://github.com/ubirch/ubirch-mbed-nacl-cm0"
            )
endif ()

set(UBIRCH_NACL_PATH ubirch-mbed-nacl-cm0)
file(GLOB_RECURSE NACL_SOURCES "${UBIRCH_NACL_PATH}/*.c")

# ubirch protocol
if (NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/ubirch-protocol)
    message(FATAL_ERROR "required library ubirch-protocol missing, checkout from github:\n"
            "$ git clone https://github.com/ubirch/ubirch-protocol"
            )
endif ()

set(UBIRCH_PROTOCOL_SRCS
        ${CMAKE_CURRENT_LIST_DIR}/ubirch-protocol/ubirch/ubirch_protocol_kex.c
        ${CMAKE_CURRENT_LIST_DIR}/ubirch-protocol/ubirch/digest/sha512.c)

# sources of our project, including dependent libraries
set(MAIN_SRCS
        main/main.c
        main/sntpTime.c
        main/wifi.c
        main/keyHandling.c
        main/util.c
        main/settings.h
        main/ubirch-proto-http.c
        ${MSGPACK_SRCS}
        ${UBIRCH_PROTOCOL_SRCS}
        )

list(APPEND MAIN_SRCS ${NACL_SOURCES})

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# now configure the project
project(example-esp32)

include_directories(
        ubirch-mbed-msgpack
        ubirch-mbed-nacl-cm0
        ubirch-mbed-nacl-cm0/source/nacl
        ubirch-mbed-nacl-cm0/source/nacl/crypto_sign
        ubirch-mbed-nacl-cm0/source/nacl/crypto_hash
        ubirch-mbed-nacl-cm0/source/nacl/crypto_hashblocks
        ubirch-mbed-nacl-cm0/source/nacl/include
        ubirch-mbed-nacl-cm0/source/nacl/crypto_verify
        ubirch-mbed-nacl-cm0/source/nacl/shared
        ubirch-mbed-nacl-cm0/source/randombytes
        ubirch-protocol/ubirch
        ubirch-protocol/ubirch/digest
        main
)
