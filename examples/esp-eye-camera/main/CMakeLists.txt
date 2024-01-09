idf_component_register(
  SRCS esp32-rtsp.cpp edgecam_esp32.c edgecam_iam.c perfmon.c
    ${CMAKE_CURRENT_LIST_DIR}/../../../nabto-embedded-sdk/apps/common/random_string.c
  INCLUDE_DIRS ""
  PRIV_INCLUDE_DIRS ""
  REQUIRES esp32-camera micro-rtsp esp_timer esp_wifi driver nvs_flash nabto_esp32_example nabto_esp32_iam nabto_esp32_util
)
