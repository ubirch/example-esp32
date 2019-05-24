set(COMPONENT_SRCS
        main.c
        sensor.c
        util.c
        ubirch-protocol-c8y/ubirch-protocol-c8y.c
        )

set(COMPONENT_ADD_INCLUDEDIRS
        "${CMAKE_CURRENT_LIST_DIR}"
        ${CMAKE_CURRENT_LIST_DIR}/ubirch-protocol-c8y)

set(COMPONENT_REQUIRES
        arduino-esp32
        ubirch-esp32-api-http
        ubirch-esp32-console
        ubirch-esp32-networking
        ubirch-esp32-ota
        ubirch-esp32-storage
        ubirch-mbed-msgpack
        ubirch-mbed-nacl-cm0
        ubirch-protocol
        ubirch-esp32-key-storage
        nvs_flash
        )
set(COMPONENT_PRIV_REQUIRES esp_http_client)

register_component()
