#ifndef _PRIVATE_KEY_H_
#define _PRIVATE_KEY_H_

#include <nabto/nabto_device.h>

#include <nn/log.h>

#include <stdbool.h>

bool load_or_create_private_key_esp32nvs(NabtoDevice* device, struct nn_log* logger);

#endif
