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
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_LOGI(TAG, "M1-M3 dev build; not hardware validated. PSRAM=%u heap=%u",
        (unsigned)esp_psram_get_size(), (unsigned)esp_get_free_heap_size());
    lv_display_t *display = NULL;
    ESP_ERROR_CHECK(sec_board_init(&display));
    ESP_ERROR_CHECK(sec_wifi_init());
    esp_err_t ble_err = sec_ble_init();
    if (ble_err != ESP_OK) ESP_LOGE(TAG, "BLE init failed: %s", esp_err_to_name(ble_err));
    sec_ui_init();
}
