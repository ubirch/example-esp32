# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)


# NOTE: This file is only used for configuring CLion, use regular `make` for building.

if (NOT ENV{IDF_PATH})
    if (EXISTS "$ENV{HOME}/.idf")
        file(STRINGS "$ENV{HOME}/.idf" IDF_PATH)
        set(ENV{IDF_PATH} ${IDF_PATH})
    else ()
        message(FATAL_ERROR "ESP-IDF: set $ESP_IDF environment variable or write path to file $ENV{HOME}/.idf")
    endif ()
endif()
message(STATUS "IDF_PATH: ${IDF_PATH}")

add_definitions(-DESP_PLATFORM -DRANDOMBYTES_DEFAULT_IMPLEMENTATION)

#set(CMAKE_VERBOSE_MAKEFILE 1)
set(MSGPACK_SRCS
        ${CMAKE_CURRENT_LIST_DIR}/ubirch-mbed-msgpack/objectc.c
        ${CMAKE_CURRENT_LIST_DIR}/ubirch-mbed-msgpack/unpack.c
        ${CMAKE_CURRENT_LIST_DIR}/ubirch-mbed-msgpack/version.c
        ${CMAKE_CURRENT_LIST_DIR}/ubirch-mbed-msgpack/vrefbuffer.c
        ${CMAKE_CURRENT_LIST_DIR}/ubirch-mbed-msgpack/zone.c)

set(UBIRCH_PROTOCOL_SRCS
        ${CMAKE_CURRENT_LIST_DIR}/ubirch-protocol/ubirch/ubirch_protocol_kex.c
        ${CMAKE_CURRENT_LIST_DIR}/ubirch-protocol/ubirch/digest/sha512.c)



set(UBIRCH_NACL_PATH ubirch-mbed-nacl-cm0)
file(GLOB_RECURSE NACL_SOURCES "${UBIRCH_NACL_PATH}/*.c")


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



