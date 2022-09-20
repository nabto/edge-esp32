idf_component_register(
  SRCS
    src/nabto_esp32_example.c
    src/nabto_init.c
    src/wifi_init.c
  INCLUDE_DIRS
    "include"
  REQUIRES nabto_device esp_wifi nvs_flash
  )
