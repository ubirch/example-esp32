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
set(ENV{IDF_PATH} /home/ESP/arduino_lib_test/esp-idf/)
message(STATUS "IDF_PATH: ${IDF_PATH}")

# add some necessary defnitions
add_definitions(-DRANDOMBYTES_DEFAULT_IMPLEMENTATION)

# set the components, EXCLUDING libsodium, because of the conflict with ubirch-mbed-nacl-cm0
set(COMPONENTS arduino;soc;log;heap;freertos;vfs;newlib;driver;ethernet;lwip;tcpip_adapter;partition_table;app_update;spi_flash;mbedtls;micro-ecc;bootloader_support;nvs_flash;pthread;smartconfig_ack;wpa_supplicant;xtensa-debug-module;esp32;cxx;app_trace;jsmn;aws_iot;bootloader;bt;coap;console;nghttp;esp-tls;esp_adc_cal;esp_http_client;esp_https_ota;esptool_py;expat;wear_levelling;sdmmc;fatfs;idf_test;json;mdns;openssl;spiffs;ulp)

list(APPEND COMPONENTS main)
#set(CMAKE_VERBOSE_MAKEFILE 1)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# now configure the project
project(example-esp32)

