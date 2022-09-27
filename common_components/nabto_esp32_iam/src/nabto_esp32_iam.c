#include "nabto_esp32_iam.h"

#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"

#include "modules/iam/nm_iam.h"
#include "modules/iam/nm_iam_serializer.h"

#include "nn/log.h"

static const char* iamStateNvsKey = "iam_state";

static const char *TAG = "nabto IAM";

static void state_changed(struct nm_iam* iam, void* userData);
static bool save_state(struct nabto_esp32_iam* esp32Iam, struct nm_iam_state* state);

static bool load_state(struct nabto_esp32_iam* esp32Iam);

void nabto_esp32_iam_init(struct nabto_esp32_iam* esp32Iam, NabtoDevice* device, struct nn_log* logger, struct nm_iam_configuration* iamConfiguration, struct nm_iam_state* defaultIamState, nvs_handle_t nvsHandle)
{
    memset(esp32Iam, 0, sizeof(struct nabto_esp32_iam));
    esp32Iam->logger = *logger;
    esp32Iam->defaultIamState = defaultIamState;
    esp32Iam->nvsHandle = nvsHandle;

    nm_iam_init(&esp32Iam->iam, device, &esp32Iam->logger);

    if (!nm_iam_load_configuration(&esp32Iam->iam, iamConfiguration)) {
        ESP_LOGE(TAG, "cannot load iam configuration");
    }

    if (load_state(esp32Iam)) {

    } else {
        ESP_LOGI(TAG, "No iam state found initialising a new one.");
        save_state(esp32Iam, esp32Iam->defaultIamState);
        if (!load_state(esp32Iam)) {
            ESP_LOGE(TAG, "Cannot load iam state which was just created.");
        }
    }

    nm_iam_set_state_changed_callback(&esp32Iam->iam, state_changed, esp32Iam);
}

bool save_state(struct nabto_esp32_iam* esp32Iam, struct nm_iam_state* state)
{
    bool status = true;
    char* jsonState = NULL;
    if (!nm_iam_serializer_state_dump_json(state, &jsonState)) {
        ESP_LOGE(TAG, "Cannot dump state as json");
        status = false;
    }

    if (status) {
        ESP_ERROR_CHECK(nvs_set_blob(esp32Iam->nvsHandle, iamStateNvsKey, jsonState, strlen(jsonState)));
    }

    nm_iam_serializer_string_free(jsonState);
    return status;
}

void state_changed(struct nm_iam* iam, void* userData)
{
    struct nabto_esp32_iam* esp32Iam = userData;
    struct nm_iam_state* state = nm_iam_dump_state(iam);
    if (state == NULL) {
        ESP_LOGE(TAG, "out of memory");
        return;
    }
    save_state(esp32Iam, state);
    nm_iam_state_free(state);
}

bool load_state(struct nabto_esp32_iam* esp32Iam)
{
    size_t requiredSize = 0;
    esp_err_t err = nvs_get_blob(esp32Iam->nvsHandle, iamStateNvsKey, NULL, &requiredSize);
    if (err != ESP_OK) {
        return false;
    }
    uint8_t* blob = calloc(1, requiredSize+1);
    if (blob == NULL)  {
        ESP_LOGE(TAG, "out of memory");
        return false;
    }
    err = nvs_get_blob(esp32Iam->nvsHandle, iamStateNvsKey, blob, &requiredSize);
    if (err != ESP_OK) {
        free(blob);
        return false;
    }

    struct nm_iam_state* state = nm_iam_state_new();
    if (state == NULL) {
        free(blob);
        ESP_LOGE(TAG, "out of memory");
        return false;
    }
    if (!nm_iam_serializer_state_load_json(state, (const char*)blob, &esp32Iam->logger)) {
        ESP_LOGE(TAG, "Cannot load iam state");
        free(blob);
        nm_iam_state_free(state);
        return false;
    }
    free(blob);

    if (!nm_iam_load_state(&esp32Iam->iam, state)) {
        nm_iam_state_free(state);
        return false;
    }

    return true;
}
