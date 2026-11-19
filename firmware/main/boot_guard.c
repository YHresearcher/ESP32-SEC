#include "boot_guard.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "SEC_BOOT_GUARD";
static const char *NAMESPACE = "sec_boot";
static const char *KEY = "incomplete";

esp_err_t sec_boot_guard_begin(bool *safe_mode, uint8_t *attempts) {
    if (!safe_mode || !attempts) return ESP_ERR_INVALID_ARG;
    *safe_mode = false;
    *attempts = 0;
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;
    uint8_t failures = 0;
    err = nvs_get_u8(handle, KEY, &failures);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(handle);
        return err;
    }
    // A fresh boot starts incomplete until the UI proves that it can tick.
    if (failures < 3) ++failures;
    *safe_mode = failures >= 2;
    *attempts = failures;
    err = nvs_set_u8(handle, KEY, failures);
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    ESP_LOGW(TAG, "boot attempt=%u safe_mode=%d reset_reason=%d",
             (unsigned)failures, (int)*safe_mode, (int)esp_reset_reason());
    return err;
}

esp_err_t sec_boot_guard_healthy(void) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;
    err = nvs_set_u8(handle, KEY, 0);
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    if (err == ESP_OK) ESP_LOGI(TAG, "UI heartbeat reached; boot guard cleared");
    return err;
}
