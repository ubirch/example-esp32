cmake_minimum_required(VERSION 3.5)

# add some necessary defnitions
add_definitions(
        -DRANDOMBYTES_DEFAULT_IMPLEMENTATION
        -DMSGPACK_ENDIAN_LITTLE_BYTE
    )

# FIXME: do we need to create links to CMakeLists.txt in all components, too?

set(EXTRA_COMPONENT_DIRS "../components")
set(PROJECT_COMPONENTS msgpack-c ubirch-mbed-nacl-cm0 ubirch-protocol)
set(EXCLUDE_COMPONENTS libsodium)

# go through the list of components and link or copy the CMakeLists.txt if necessary
foreach (C_DIR ${PROJECT_COMPONENTS})
    if (NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/../components/${C_DIR}/CMakeLists.txt")
        if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/../components/${C_DIR}/esp32.cmake")
            if (CMAKE_HOST_UNIX)
                set(LINK_OR_COPY create_symlink)
            else ()
                set(LINK_OR_COPY copy)
            endif ()

            set(ESP32CMAKE ${CMAKE_CURRENT_LIST_DIR}/../components/${C_DIR}/esp32.cmake)
            set(CMAKELISTSTXT ${CMAKE_CURRENT_LIST_DIR}/../components/${C_DIR}/CMakeLists.txt)
            execute_process(COMMAND "${CMAKE_COMMAND}" -E ${LINK_OR_COPY} "${ESP32CMAKE}" "${CMAKELISTSTXT}"
                    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../components/${C_DIR}")
        endif ()
    endif ()
endforeach ()

set(TEST_COMPONENTS "ubirch-esp32-api-http"
                    "ubirch-esp32-console"
                    "ubirch-esp32-key-storage"
                    "ubirch-esp32-networking"
                    "ubirch-esp32-ota"
                    "ubirch-esp32-storage"
                    "ubirch-mbed-nacl-cm0"
                    "ubirch-protocol"
                    CACHE STRING "List of components to test")

set(SDKCONFIG_DEFAULTS "./sdkconfig.test" "../sdkconfig.defaults")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(example_esp32_test)
