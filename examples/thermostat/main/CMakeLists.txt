idf_component_register(
  SRCS
    "thermostat.c"
    "thermostat_iam.c"
    ${CMAKE_CURRENT_LIST_DIR}/../../../nabto-embedded-sdk/apps/common/random_string.c
  REQUIRES
    nabto_esp32_example nabto_esp32_iam
)
