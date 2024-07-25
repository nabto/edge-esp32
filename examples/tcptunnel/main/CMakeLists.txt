idf_component_register(
  SRCS
    "tcptunnel_esp32.c"
    "tcptunnel_iam.c"
    "simple_webserver.c"
    "simple_perf.c"
    #"print_stats.c"
    ${CMAKE_CURRENT_LIST_DIR}/../../../nabto-embedded-sdk/apps/common/random_string.c
  REQUIRES
    nabto_esp32_example nabto_esp32_iam nabto_esp32_util driver esp_http_server esp_timer
)

add_compile_options (-fdiagnostics-color=never)
