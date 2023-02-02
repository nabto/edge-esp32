#ifndef _NABTO_ESP32_EXAMPLE_H_
#define _NABTO_ESP32_EXAMPLE_H_

#include <nabto/nabto_device.h>

#include "nvs_flash.h"

NabtoDeviceError nabto_esp32_example_set_ids(NabtoDevice* device);

void nabto_esp32_example_init_wifi();


#endif
