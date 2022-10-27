#ifndef _NABTO_ESP32_EXAMPLE_H_
#define _NABTO_ESP32_EXAMPLE_H_

#include <nabto/nabto_device.h>

#include "nvs_flash.h"

void nabto_esp32_example_init();

NabtoDevice* nabto_esp32_example_get_device();

NabtoDeviceError nabto_esp32_example_set_ids(NabtoDevice* device);

bool nabto_esp32_example_load_private_key(NabtoDevice* device, nvs_handle_t nvsHandle);

void nabto_esp32_example_init_wifi();


#endif
