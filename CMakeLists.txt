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
add_definitions(-DRANDOMBYTES_DEFAULT_IMPLEMENTATION)

# set the components, EXCLUDING libsodium, because of the conflict with ubirch-mbed-nacl-cm0
set(COMPONENTS soc;log;heap;freertos;vfs;newlib;driver;ethernet;lwip;tcpip_adapter;partition_table;app_update;spi_flash;mbedtls;micro-ecc;bootloader_support;nvs_flash;pthread;smartconfig_ack;wpa_supplicant;xtensa-debug-module;esp32;cxx;app_trace;jsmn;aws_iot;bootloader;bt;coap;console;nghttp;esp-tls;esp_adc_cal;esp_http_client;esp_https_ota;esptool_py;expat;wear_levelling;sdmmc;fatfs;idf_test;json;mdns;openssl;spiffs;ulp)

# set the components local to our project
set(PROJECT_COMPONENTS
        ubirch-esp32-networking
        ubirch-mbed-msgpack
        ubirch-mbed-nacl-cm0
        ubirch-protocol
        arduino-esp32
        )

# go through the list of components and link or copy the CMakeLists.txt if necessary
foreach (C_DIR ${PROJECT_COMPONENTS})
    if (NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/components/${C_DIR}/CMakeLists.txt")
        if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/components/${C_DIR}/esp32.cmake")
            if (CMAKE_HOST_UNIX)
                set(LINK_OR_COPY create_symlink)
            else ()
                set(LINK_OR_COPY copy)
            endif ()

            set(ESP32CMAKE ${CMAKE_CURRENT_LIST_DIR}/components/${C_DIR}/esp32.cmake)
            set(CMAKELISTSTXT ${CMAKE_CURRENT_LIST_DIR}/components/${C_DIR}/CMakeLists.txt)
            execute_process(COMMAND "${CMAKE_COMMAND}" -E ${LINK_OR_COPY} "${ESP32CMAKE}" "${CMAKELISTSTXT}"
                    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/components/${C_DIR}")
        endif ()
    else ()
        list(APPEND COMPONENTS ${C_DIR})
    endif ()
endforeach ()
list(APPEND COMPONENTS main)
#set(CMAKE_VERBOSE_MAKEFILE 1)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# now configure the project
project(example-esp32)
