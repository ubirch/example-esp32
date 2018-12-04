# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

#set(CMAKE_VERBOSE_MAKEFILE 1)
#set(DEBUG 1)

# check and set the IDF_PATH
message(STATUS "$ENV{IDF_PATH}")
if (NOT DEFINED ENV{IDF_PATH})
    if (EXISTS "$ENV{HOME}/.idf")
        file(STRINGS "$ENV{HOME}/.idf" IDF_PATH)
        set(ENV{IDF_PATH} ${IDF_PATH})
    else ()
        message(FATAL_ERROR "ESP-IDF: set $IDF_PATH environment variable or write path to file $ENV{HOME}/.idf")
    endif ()
else ()
    set(IDF_PATH $ENV{IDF_PATH})
endif ()
message(STATUS "IDF_PATH: ${IDF_PATH}")

# add some necessary defnitions
add_definitions(-DRANDOMBYTES_DEFAULT_IMPLEMENTATION)

set(EXCLUDE_COMPONENTS libsodium)

# set the components local to our project
set(PROJECT_COMPONENTS ubirch-mbed-msgpack ubirch-mbed-nacl-cm0 ubirch-protocol)
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
    endif ()
endforeach ()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# now configure the project
project(example-esp32)
