SET(MSGPACK_ENDIAN_BIG_BYTE 0)
SET(MSGPACK_ENDIAN_LITTLE_BYTE 1)

CONFIGURE_FILE (
    ${CMAKE_CURRENT_LIST_DIR}/msgpack-c/cmake/sysdep.h.in
    ${CMAKE_CURRENT_LIST_DIR}/msgpack-c/include/msgpack/sysdep.h
    @ONLY
)

CONFIGURE_FILE (
    ${CMAKE_CURRENT_LIST_DIR}/msgpack-c/cmake/pack_template.h.in
    ${CMAKE_CURRENT_LIST_DIR}/msgpack-c/include/msgpack/pack_template.h
    @ONLY
)

set(COMPONENT_SRCDIRS msgpack-c/src)
set(COMPONENT_ADD_INCLUDEDIRS ${CMAKE_CURRENT_LIST_DIR}/msgpack-c/include)

register_component()
