
# This common component is the basic integration of Nabto edge with the ESP32 idf
set(EXTRA_COMPONENT_DIRS ../../components ../components)

cmake_minimum_required(VERSION 3.5)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp-eye-camera)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_ARPA_INET_H")
