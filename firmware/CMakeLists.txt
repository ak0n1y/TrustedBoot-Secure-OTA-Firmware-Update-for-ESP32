cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED PROJECT_VER)
    set(PROJECT_VER "1.0.0")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32-secure-ota)
