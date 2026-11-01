cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(streamoo)

# These documented comparison directories have a fixed meaning, even after
# fullclean. A directory name alone does not select an ESP-IDF sdkconfig.
get_filename_component(streamoo_build_name "${CMAKE_BINARY_DIR}" NAME)
set(streamoo_expected_backend "")
if(streamoo_build_name STREQUAL "flac-dr")
    set(streamoo_expected_backend DR)
elseif(streamoo_build_name STREQUAL "flac-micro")
    set(streamoo_expected_backend MICRO)
elseif(streamoo_build_name STREQUAL "flac-esp")
    set(streamoo_expected_backend ESPRESSIF)
endif()
if(streamoo_expected_backend AND NOT CONFIG_STREAMOO_FLAC_${streamoo_expected_backend})
    message(FATAL_ERROR
        "Wrong FLAC backend for ${streamoo_build_name}. Use python3 tools/build_flac.py dr|micro|esp build or flash; -B alone does not select sdkconfig.")
endif()

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
