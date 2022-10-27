#include "thermostat_state_esp32.h"
#include "thermostat_led.h"

#include "esp_log.h"

#include <cjson/cJSON.h>

static const char* TAG = "thermostat";

// The name of the termostat state inside nvs, the name should be shorter than 15 characters.
static const char* NVS_THERMOSTAT_STATE_NAME = "ts_state";

static bool get_power(void* impl);
static void set_power(void* impl, bool power);
static double get_target(void* impl);
static void set_target(void* impl, double target);
static double get_temperature(void* impl);
static void set_mode(void* impl, enum thermostat_mode mode);
static enum thermostat_mode get_mode(void* impl);

static void save_state(struct thermostat_state_esp32* tse);

static void thermostat_state_esp32_create_default_state(struct thermostat_state_esp32* fb);
static bool thermostat_state_esp32_load_data(struct thermostat_state_esp32* tse, struct nn_log* logger);

void thermostat_state_esp32_init(struct thermostat_state_esp32* tse, struct thermostat_state* thermostatState, nvs_handle_t nvsHandle, struct nn_log* logger)
{
    tse->nvsHandle = nvsHandle;
    tse->stateData.mode = THERMOSTAT_MODE_HEAT;
    tse->stateData.temperature = 22.3;
    tse->stateData.target = 22.3;
    tse->stateData.power = false;

    thermostatState->impl = tse;
    thermostatState->get_power = get_power;
    thermostatState->set_power = set_power;
    thermostatState->get_target = get_target;
    thermostatState->set_target = set_target;
    thermostatState->get_temperature = get_temperature;
    thermostatState->get_mode = get_mode;
    thermostatState->set_mode = set_mode;

    thermostat_state_esp32_load_data(tse, logger);
    thermostat_led_init();
    thermostat_led_update(&tse->stateData);
}

void thermostat_state_file_backend_deinit(struct thermostat_state_esp32* fileBackend)
{
}

/**
 * Called initially to load state data in from a file
 */
bool thermostat_state_esp32_load_data(struct thermostat_state_esp32* tse, struct nn_log* logger)
{
    bool status = true;
    size_t stateLength = 0;
    esp_err_t err = nvs_get_blob(tse->nvsHandle, NVS_THERMOSTAT_STATE_NAME, NULL, &stateLength);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        thermostat_state_esp32_create_default_state(tse);
    }
    err = nvs_get_blob(tse->nvsHandle, NVS_THERMOSTAT_STATE_NAME, NULL, &stateLength);
    if (err != ESP_OK) {
        return false;
    }
    uint8_t* blob = calloc(1, stateLength);
    err = nvs_get_blob(tse->nvsHandle, NVS_THERMOSTAT_STATE_NAME, blob, &stateLength);
    if (err != ESP_OK) {
        return false;
    }
    cJSON* json;
    json = cJSON_ParseWithLength((const char*)blob, stateLength);
    if (json == NULL) {
        free(blob);
        ESP_LOGE(TAG, "Cannot parse saved thermostat state");
        return false;
    }

    status = thermostat_state_data_decode_from_json(json, &tse->stateData, logger);
    cJSON_Delete(json);
    free(blob);
    return status;
}

bool get_power(void* impl)
{
    struct thermostat_state_esp32* fb = impl;
    return fb->stateData.power;
}

void set_power(void* impl, bool power)
{
    struct thermostat_state_esp32* fb = impl;
    fb->stateData.power = power;
    save_state(fb);
}
double get_target(void* impl)
{
    struct thermostat_state_esp32* fb = impl;
    return fb->stateData.target;

}
void set_target(void* impl, double target)
{
    struct thermostat_state_esp32* fb = impl;
    fb->stateData.target = target;
    save_state(fb);
}
double get_temperature(void* impl)
{
    struct thermostat_state_esp32* fb = impl;
    return fb->stateData.temperature;

}
void set_mode(void* impl, enum thermostat_mode mode)
{
    struct thermostat_state_esp32* fb = impl;
    fb->stateData.mode = mode;
    save_state(fb);
}
enum thermostat_mode get_mode(void* impl)
{
    struct thermostat_state_esp32* fb = impl;
    return fb->stateData.mode;

}

const char* power_as_string(bool power)
{
    if (power) {
        return "ON";
    } else {
        return "OFF";
    }
}

void save_state(struct thermostat_state_esp32* tse)
{
    ESP_LOGI(TAG, "Thermostat state updated: Target temperature %.2f, Mode: %s, Power: %s, ", tse->stateData.target, thermostat_state_mode_as_string(tse->stateData.mode), power_as_string(tse->stateData.power));
    thermostat_led_update(&tse->stateData);
    cJSON* j = NULL;
    char* buffer = NULL;
    esp_err_t nvsStatus = ESP_FAIL;

    j = thermostat_state_data_encode_as_json(&tse->stateData);
    if (j != NULL) {
        buffer = cJSON_PrintUnformatted(j);
    }

    if (buffer != NULL) {
        nvsStatus = nvs_set_blob(tse->nvsHandle, NVS_THERMOSTAT_STATE_NAME, buffer, strlen(buffer));
        if (nvsStatus != ESP_OK) {
            ESP_LOGE(TAG, "cannot write thermostat state %s", esp_err_to_name(nvsStatus));
        }
    }
    free(buffer);
    cJSON_Delete(j);
}

void thermostat_state_esp32_create_default_state(struct thermostat_state_esp32* fb)
{
    fb->stateData.mode = THERMOSTAT_MODE_HEAT;
    fb->stateData.temperature = 22.3;
    fb->stateData.target = 22.3;
    fb->stateData.power = false;
    save_state(fb);
}


static double smoothstep(double start, double end, double x)
{
    if (x < start) {
        return 0;
    }

    if (x >= end) {
        return 1;
    }

    x = (x - start) / (end - start);
    return x * x * (3 - 2 * x);
}

void thermostat_state_file_backend_update(struct thermostat_state_esp32* fb, double deltaTime) {
    if (fb->stateData.power) {
        // Move temperature smoothly towards target
        // When temperature is within smoothingDistance units of target
        // the temperature will start to move more slowly the closer it gets to target
        double speed = 5 * deltaTime;
        double smoothingDistance = 20.0;

        double start = fb->stateData.temperature;
        double end = fb->stateData.target;
        double signedDistance = end - start;
        double distance = (signedDistance > 0) ? signedDistance : -signedDistance;
        double sign = (signedDistance >= 0) ? 1 : -1;

        fb->stateData.temperature += sign * speed * smoothstep(0, smoothingDistance, distance);
    }
}
