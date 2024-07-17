# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# This common component is the basic integration of Nabto edge with the ESP32 idf
# Also this example uses an extra component for common functions such as Wi-Fi and Ethernet connection.
#set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)
set(EXTRA_COMPONENT_DIRS ../../components ../components) 
#set(EXTRA_COMPONENT_DIRS ../components $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)

add_compile_options (-fdiagnostics-color=never)

# include ADF stuff 
include($ENV{ADF_PATH}/CMakeLists.txt)

# IDF standard include
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Specify additional directories to look for cmake files
list(APPEND EXTRA_COMPONENT_DIRS $ENV{ADF_PATH}/components)

project(tcp_server_stream)
