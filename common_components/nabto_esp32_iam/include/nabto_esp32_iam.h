#pragma once

#include "modules/iam/nm_iam.h"

#include "nvs_flash.h"

struct nabto_esp32_iam {
    struct nn_log logger;
    struct nm_iam iam;
    struct nm_iam_state* defaultIamState;
    struct nm_iam_configuration* iamConfiguration;
    nvs_handle_t nvsHandle;
};

void nabto_esp32_iam_init(struct nabto_esp32_iam* esp32Iam, NabtoDevice* device);
