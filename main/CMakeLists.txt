
#set the project root directory
set(PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)

# msgpack implementation
if (NOT EXISTS ${PROJECT_ROOT}/ubirch-mbed-msgpack)
    message(FATAL_ERROR "required library ubirch-mbed-msgpack missing, checkout from github:\n"
            "$ git clone https://github.com/ubirch/ubirch-mbed-msgpack"
            )
endif ()

set(MSGPACK_SRCS
        ${PROJECT_ROOT}/ubirch-mbed-msgpack/objectc.c
        ${PROJECT_ROOT}/ubirch-mbed-msgpack/unpack.c
        ${PROJECT_ROOT}/ubirch-mbed-msgpack/version.c
        ${PROJECT_ROOT}/ubirch-mbed-msgpack/vrefbuffer.c
        ${PROJECT_ROOT}/ubirch-mbed-msgpack/zone.c)

# ED25519 implementation
if (NOT EXISTS ${PROJECT_ROOT}/ubirch-mbed-nacl-cm0)
    message(FATAL_ERROR "required library ubirch-mbed-nacl-cm0 missing, checkout from github:\n"
            "$ git clone https://github.com/ubirch/ubirch-mbed-nacl-cm0"
            )
endif ()

set(UBIRCH_NACL_PATH ${PROJECT_ROOT}/ubirch-mbed-nacl-cm0)
file(GLOB_RECURSE NACL_SRCS "${UBIRCH_NACL_PATH}/*.c")

# ubirch protocol
if (NOT EXISTS ${PROJECT_ROOT}/ubirch-protocol)
    message(FATAL_ERROR "required library ubirch-protocol missing, checkout from github:\n"
            "$ git clone https://github.com/ubirch/ubirch-protocol"
            )
endif ()

set(UBIRCH_PROTOCOL_SRCS
        ${PROJECT_ROOT}/ubirch-protocol/ubirch/ubirch_protocol_kex.c
        ${PROJECT_ROOT}/ubirch-protocol/ubirch/digest/sha512.c)

#ARDUINO hal

#set(UBIRCH_HAL_PATH ${PROJECT_ROOT}/esp32)
#file(GLOB_RECURSE HAL_SRCS "${UBIRCH_HAL_PATH}/*.c")


set(COMPONENT_SRCS
        main.c
        sntp_time.c
        wifi.c
        key_handling.c
        util.c
        settings.h
        ubirch_proto_http.c
        ${MSGPACK_SRCS}
        ${UBIRCH_PROTOCOL_SRCS}
        ${NACL_SRCS}
        ${HAL_SRCS}
        cmd_system.c
        ubirch_console.c
        console_cmd.c
        storage.c
        temp_temp.c
        )


set(COMPONENT_ADD_INCLUDEDIRS
        ${PROJECT_ROOT}/ubirch-mbed-msgpack
        ${PROJECT_ROOT}/ubirch-mbed-nacl-cm0
        ${PROJECT_ROOT}/ubirch-mbed-nacl-cm0/source/nacl
        ${PROJECT_ROOT}/ubirch-mbed-nacl-cm0/source/nacl/crypto_sign
        ${PROJECT_ROOT}/ubirch-mbed-nacl-cm0/source/nacl/crypto_hash
        ${PROJECT_ROOT}/ubirch-mbed-nacl-cm0/source/nacl/crypto_hashblocks
        ${PROJECT_ROOT}/ubirch-mbed-nacl-cm0/source/nacl/include
        ${PROJECT_ROOT}/ubirch-mbed-nacl-cm0/source/nacl/crypto_verify
        ${PROJECT_ROOT}/ubirch-mbed-nacl-cm0/source/nacl/shared
        ${PROJECT_ROOT}/ubirch-mbed-nacl-cm0/source/randombytes
        ${PROJECT_ROOT}/ubirch-protocol/ubirch
        ${PROJECT_ROOT}/ubirch-protocol/ubirch/digest
        ${CMAKE_CURRENT_LIST_DIR}
        /.
        )

register_component()
