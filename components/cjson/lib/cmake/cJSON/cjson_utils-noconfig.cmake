#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "cjson_utils" for configuration ""
set_property(TARGET cjson_utils APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(cjson_utils PROPERTIES
        IMPORTED_LINK_INTERFACE_LIBRARIES_NOCONFIG "cjson"
        IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libcjson_utils.so.1.7.1"
        IMPORTED_SONAME_NOCONFIG "libcjson_utils.so.1"
        )

list(APPEND _IMPORT_CHECK_TARGETS cjson_utils)
list(APPEND _IMPORT_CHECK_FILES_FOR_cjson_utils "${_IMPORT_PREFIX}/lib/libcjson_utils.so.1.7.1")

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
