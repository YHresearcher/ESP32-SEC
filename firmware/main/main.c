#include "board.h"
#include "ui.h"
#include "boot_guard.h"
#include "wifi_scan.h"
#include "ble_scan.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_psram.h"
#include "nvs_flash.h"

static const char *TAG = "ESP32_SEC";

void app_main(void) {
    ESP_LOGI(TAG, "SEC_BOOT_STAGE_1: NVS");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    bool recovery_mode = false;
    uint8_t attempts = 0;
    esp_err_t guard_err = sec_boot_guard_begin(&recovery_mode, &attempts);
    if (guard_err != ESP_OK) {
        ESP_LOGW(TAG, "Boot guard unavailable: %s", esp_err_to_name(guard_err));
        // Never assume an incomplete flash/NVS read is proof of safe hardware.
        recovery_mode = true;
    }
    ESP_LOGI(TAG, "SEC_BOOT_STAGE_2: PSRAM=%u heap=%u",
        (unsigned)esp_psram_get_size(), (unsigned)esp_get_free_heap_size());

    // Bring up the LCD and show a UI BEFORE starting the radios.
    lv_display_t *display = NULL;
    ESP_LOGI(TAG, "SEC_BOOT_STAGE_3: LCD + touch");
    err = sec_board_init(&display, recovery_mode);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SEC_LCD_INIT_FAILED: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "SEC_BOOT_STAGE_4: render UI");
    sec_ui_init(recovery_mode);
    if (recovery_mode) {
        ESP_LOGW(TAG, "SEC_RECOVERY_ACTIVE: radios disabled. USB serial ready.");
        return;
    }

    ESP_LOGI(TAG, "SEC_BOOT_STAGE_5: Wi-Fi init");
    err = sec_wifi_init();
    if (err != ESP_OK) ESP_LOGE(TAG, "SEC_WIFI_INIT_FAILED: %s", esp_err_to_name(err));

    ESP_LOGI(TAG, "SEC_BOOT_STAGE_6: BLE init");
    err = sec_ble_init();
    if (err != ESP_OK) ESP_LOGE(TAG, "SEC_BLE_INIT_FAILED: %s", esp_err_to_name(err));
    ESP_LOGI(TAG, "SEC_BOOT_READY");
}
