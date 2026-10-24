cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(streamoo)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
             "${CMAKE_CURRENT_LIST_DIR}/tools/check_config.py")

if(NOT IDF_VERSION_MAJOR EQUAL 6 OR NOT IDF_VERSION_MINOR EQUAL 1)
    message(FATAL_ERROR "StreamOO requires ESP-IDF 6.1; found ${IDF_VERSION}")
endif()
if(NOT IDF_TARGET STREQUAL "esp32s3")
    message(FATAL_ERROR "StreamOO N16R8 requires target esp32s3")
endif()

execute_process(COMMAND "${PYTHON}" "${CMAKE_CURRENT_LIST_DIR}/tools/check_config.py"
                "${SDKCONFIG}" --platform RESULT_VARIABLE STREAMOO_CONFIG_RESULT)
if(NOT STREAMOO_CONFIG_RESULT EQUAL 0)
    message(FATAL_ERROR "Invalid StreamOO GPIO configuration; see diagnostics above")
endif()
