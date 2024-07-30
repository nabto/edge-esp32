

idf_component_register(SRCS audio_demo.c audio_server.c edgecam_iam.c ${CMAKE_CURRENT_LIST_DIR}/../../../nabto-embedded-sdk/apps/common/random_string.c
                    INCLUDE_DIRS "."
                    REQUIRES tcp_server_stream audio_pipeline audio_hal audio_stream esp_peripherals
                             nvs_flash nabto_esp32_example nabto_esp32_iam nabto_esp32_util)

