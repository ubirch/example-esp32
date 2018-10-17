# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)


# NOTE: This file is only used for configuring CLion, use regular `make` for building.

set (IDF_PATH "/media/wowa/5e81bf61-49cb-4835-81f4-f6f58647d624/ESP/esp-idf-v3.0.2/")

project(example-esp32)

#set(CMAKE_SYSTEM_NAME xtensa-esp32-elf)
#set(CMAKE_C_COMPILER xtensa-esp32-elf-gcc)
#set(CMAKE_FIND_ROOT_PATH /media/wowa/5e81bf61-49cb-4835-81f4-f6f58647d624/ESP/source/)

#set(CMAKE_CXX_STANDARD 11)


file(GLOB_RECURSE SOURCES "${IDF_PATH}/*.c")

set(COMPONENTS
        app_update
        bootloader
        bootloader_support
        bt
        coap
        cxx
        driver
        esp32
        esptool_py
        ethernet
        expat
        fatfs
        freertos
        idf_test
        json
        log
        lwip
        mbedtls
        micro-ecc
        newlib
        nghttp
        nvs_flash
        openssl
        partition_table
        sdmmc
        spi_flash
        soc
        tcpip_adapter
        ulp
        vfs
        wpa_supplicant
        xtensa-debug-module)

set (INCLUDE_DIRS "")
foreach (component ${COMPONENTS})
    list (APPEND INCLUDE_DIRS ${IDF_PATH}/components/${component}/include)
endforeach()
list(REMOVE_DUPLICATES INCLUDE_DIRS)
include_directories(${INCLUDE_DIRS})
include_directories("${IDF_PATH}/components")
include_directories("${IDF_PATH}/components/soc/esp32/include/soc")
include_directories("${IDF_PATH}/components/lwip/include/lwip")
include_directories("${IDF_PATH}/components/esp_http_client")
include_directories("${IDF_PATH}/components/esp_http_client/include")
include_directories("${IDF_PATH}/components/esp_http_client/lib")
include_directories("${IDF_PATH}/components/esp_http_client/lib/include")
include_directories("../ubirch-protocol/ubirch")
include_directories("../ubirch-mbed-msgpack")
include_directories("../ubirch-mbed-nacl-cm0")
include_directories("../ubirch-mbed-nacl-cm0/source/nacl")
include_directories("./main")

#include_directories("components/Arduino-Temperature-Control-Library/")

set(SOURCE_FILES
        main/main.c
        main/sntpTime.c
        main/wifi.c
        main/keyHandling.c
        main/util.c
        main/networkUtils.c
        main/settings.h
        main/ubirch-proto-http.c
        )
list (APPEND SOURCE_FILES ${SOURCES})

add_executable(esp_test ${SOURCE_FILES})



