#include "board.h"
#include "ui.h"
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

    ESP_LOGI(TAG, "SEC_BOOT_STAGE_2: PSRAM=%u heap=%u",
        (unsigned)esp_psram_get_size(), (unsigned)esp_get_free_heap_size());

    // Bring up the LCD and show a UI BEFORE starting the radios.
    lv_display_t *display = NULL;
    ESP_LOGI(TAG, "SEC_BOOT_STAGE_3: LCD + touch");
    err = sec_board_init(&display);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SEC_LCD_INIT_FAILED: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "SEC_BOOT_STAGE_4: render UI");
    sec_ui_init();

    ESP_LOGI(TAG, "SEC_BOOT_STAGE_5: Wi-Fi init");
    err = sec_wifi_init();
    if (err != ESP_OK) ESP_LOGE(TAG, "SEC_WIFI_INIT_FAILED: %s", esp_err_to_name(err));

    ESP_LOGI(TAG, "SEC_BOOT_STAGE_6: BLE init");
    err = sec_ble_init();
    if (err != ESP_OK) ESP_LOGE(TAG, "SEC_BLE_INIT_FAILED: %s", esp_err_to_name(err));
    ESP_LOGI(TAG, "SEC_BOOT_READY");
}
