set(COMPONENT_SRCS
        main.c
        sensor.c
        )

set(COMPONENT_ADD_INCLUDEDIRS "${CMAKE_CURRENT_LIST_DIR}")

set(COMPONENT_REQUIRES
        ubirch-esp32-console
        ubirch-esp32-networking
        ubirch-esp32-ota
        ubirch-esp32-storage
        msgpack-c
        ubirch-mbed-nacl-cm0
        ubirch-protocol
        ubirch-esp32-api-http
        ubirch-esp32-key-storage
        nvs_flash
        )

register_component()
