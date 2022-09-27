include(${CMAKE_CURRENT_LIST_DIR}/../../nabto-embedded-sdk/nabto_primary_files.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../nabto-embedded-sdk/nabto_modules_files.cmake)


idf_component_register(
  SRCS
    src/nabto_esp32_util_nn_log.c
  INCLUDE_DIRS
    ${ne_nn_include_dirs}
    include
  REQUIRES
    nabto_device
)
