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
endif ()
message(STATUS "IDF_PATH: ${IDF_PATH}")

# add some necessary defnitions
add_definitions(-DRANDOMBYTES_DEFAULT_IMPLEMENTATION)

# set the components, EXCLUDING libsodium, because of the conflict with  ubirch-mbed-nacl-cm0;
set(COMPONENTS soc;log;heap;freertos;vfs;newlib;esp_ringbuf;driver;esp_event;ethernet;tcpip_adapter;lwip;mbedtls;micro-ecc;bootloader_support;nvs_flash;xtensa-debug-module;app_trace;esp32;bootloader;bt;coap;console; esp_http_client; esptool_py; idf_test; ulp;wifi_provisioning; ethernet; partition_table;app_update;spi_flash; pthread;smartconfig_ack;wpa_supplicant; cxx;asio;jsmn;aws_iot; nghttp;esp-tls;esp_adc_cal;tcp_transport; esp_http_server;esp_https_ota; expat;wear_levelling;sdmmc;fatfs;freemodbus; json;mdns;mqtt;openssl;protobuf-c;protocomm;spiffs;)
#---------------x---x---x----x--------x---x------x-----------x------x---------x--------x-----------------------------------------------------------------x-------------------x-------------------------------------------------------------------------------------------------------
list(APPEND COMPONENTS main)
#set(CMAKE_VERBOSE_MAKEFILE 1)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# now configure the project
project(example-esp32)

