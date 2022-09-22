include(${CMAKE_CURRENT_LIST_DIR}/../../nabto-embedded-sdk/nabto_primary_files.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../nabto-embedded-sdk/nabto_modules_files.cmake)


idf_component_register(
  SRCS
    ${ne_iam_src}
    ${ne_cjson_src}
    src/nabto_esp32_iam.c
  INCLUDE_DIRS
    ${ne_priv_include_dirs_no_tls}
    ${ne_include_dirs}
    ${ne_iam_include_dirs}
    ${ne_cjson_include_dir}
    ${CMAKE_CURRENT_LIST_DIR}/../../nabto-embedded-sdk
    include
  REQUIRES
    nabto_device nvs_flash
)
