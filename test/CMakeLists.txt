# This is the project CMakeLists.txt file for the test subproject
cmake_minimum_required(VERSION 3.5)

# Include the components directory of the main application:
#
set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/tools/unit-test-app/components/unity)

# Set the components to include the tests for.
# This can be overriden from CMake cache:
# - when invoking CMake directly: cmake -D TEST_COMPONENTS="xxxxx" ..
# - when using idf.py: idf.py build -T xxxxx
#
set(TEST_COMPONENTS "decke" CACHE STRING "List of components to test")

set(COMPONENT_REQUIRES unity)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(main_test)

