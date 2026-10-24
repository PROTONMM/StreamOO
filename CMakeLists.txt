cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(streamoo)

execute_process(COMMAND "${PYTHON}" "${CMAKE_CURRENT_LIST_DIR}/tools/check_config.py"
                "${SDKCONFIG}" RESULT_VARIABLE STREAMOO_CONFIG_RESULT)
if(NOT STREAMOO_CONFIG_RESULT EQUAL 0)
    message(FATAL_ERROR "Invalid StreamOO GPIO configuration; see diagnostics above")
endif()
