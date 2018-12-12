set(COMPONENT_SRCS
        main.c
        sensor.c
        util.c
        oled.c
        )

set(COMPONENT_ADD_INCLUDEDIRS "${CMAKE_CURRENT_LIST_DIR}")

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
        nvs_flash
        esp32-i2c-ssd1306-oled
        ubirch-esp32-key-storage
        )

register_component()
