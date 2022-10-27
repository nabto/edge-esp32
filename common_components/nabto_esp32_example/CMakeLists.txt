idf_component_register(
  SRCS
    src/nabto_esp32_example.c
    src/device_event_handler.c
    src/connection_event_handler.c
    #src/nabto_init.c
    src/wifi_init.c
  INCLUDE_DIRS
    "include"
  REQUIRES nabto_device esp_wifi nvs_flash
  )
