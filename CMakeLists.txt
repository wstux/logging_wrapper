cmake_minimum_required (VERSION 3.10)

################################################################################
# Project definition
################################################################################

project(logging_wrapper
        VERSION 0.0.1
        LANGUAGES CXX C)

################################################################################
# Init cmake modules path
################################################################################

set(COMMON_CMAKE_DIR cmake)
set(COMMON_MAIN_CMAKE ${COMMON_CMAKE_DIR}/main.cmake)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/${COMMON_CMAKE_DIR} ${CMAKE_MODULE_PATH})

include(${COMMON_MAIN_CMAKE} PUBLIC)

################################################################################
# Include source code
################################################################################

add_subdirectory(externals)
add_subdirectory(src)

