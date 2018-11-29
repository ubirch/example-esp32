set(COMPONENT_SRCS
        main.c
        key_handling.c
        util.c
        settings.h
        ubirch_proto_http.c
        cmd_system.c
        ubirch_console.c
        console_cmd.c
        storage.c
        temp_temp.c
        )

set(COMPONENT_ADD_INCLUDEDIRS "${CMAKE_CURRENT_LIST_DIR}")

set(COMPONENT_DEPENDS
        arduino-esp32
        ubirch-esp32-networking
        ubirch-mbed-msgpack
        ubirch-mbed-nacl-cm0
        ubirch-protocol
        )

register_component()
