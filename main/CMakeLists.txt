set(COMPONENT_SRCS
        main.c
        key_handling.c
        util.c
        temp_temp.c
        )

set(COMPONENT_ADD_INCLUDEDIRS "${CMAKE_CURRENT_LIST_DIR}")

set(COMPONENT_DEPENDS
        arduino-esp32
        ubirch-esp32-api-http
        ubirch-esp32-console
        ubirch-esp32-networking
        ubirch-esp32-storage
        ubirch-mbed-msgpack
        ubirch-mbed-nacl-cm0
        ubirch-protocol
        )

register_component()
