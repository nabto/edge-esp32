#pragma once

#include "nvs_flash.h"

#include "thermostat_state_esp32.h"
#include "thermostat_state.h"

#include "thermostat_state_data.h"

struct thermostat_state_esp32 {
    nvs_handle_t nvsHandle;
    struct thermostat_state_data stateData;
};

void thermostat_state_esp32_init(struct thermostat_state_esp32* tse, struct thermostat_state* state, nvs_handle_t nvsHandle, struct nn_log* logger);
