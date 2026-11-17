#include "wifi_scan.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include <stdio.h>
#include <string.h>

static bool s_ready = false;
esp_err_t sec_wifi_init(void) {
    if (s_ready) return ESP_OK;
    esp_err_t e = esp_netif_init();
    if (e != ESP_OK && e != ESP_ERR_INVALID_STATE) return e;
    e = esp_event_loop_create_default();
    if (e != ESP_OK && e != ESP_ERR_INVALID_STATE) return e;
    if (!esp_netif_create_default_wifi_sta()) return ESP_FAIL;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    e = esp_wifi_init(&cfg);
    if (e != ESP_OK) return e;
    e = esp_wifi_set_mode(WIFI_MODE_STA);
    if (e != ESP_OK) return e;
    e = esp_wifi_start();
    if (e == ESP_OK) s_ready = true;
    return e;
}
esp_err_t sec_wifi_scan(sec_wifi_ap_t *out, uint16_t *count) {
    if (!s_ready || !out || !count) return ESP_ERR_INVALID_STATE;
    *count = 0;
    // Only ordinary active AP discovery; no raw frame transmission or disruption.
    wifi_scan_config_t cfg = { .show_hidden = true };
    esp_err_t e = esp_wifi_scan_start(&cfg, true);
    if (e != ESP_OK) return e;
    wifi_ap_record_t records[SEC_MAX_APS] = {0};
    uint16_t found = SEC_MAX_APS;
    e = esp_wifi_scan_get_ap_records(&found, records);
    if (e != ESP_OK) return e;
    for (uint16_t i=0; i<found; ++i) {
        memcpy(out[i].ssid, records[i].ssid, 32);
        out[i].ssid[32] = 0;
        snprintf(out[i].bssid, sizeof(out[i].bssid),
            "%02X:%02X:%02X:%02X:%02X:%02X",
            records[i].bssid[0], records[i].bssid[1], records[i].bssid[2],
            records[i].bssid[3], records[i].bssid[4], records[i].bssid[5]);
        out[i].channel = records[i].primary;
        out[i].rssi = records[i].rssi;
        out[i].authmode = (int)records[i].authmode;
    }
    *count = found;
    return ESP_OK;
}

esp_err_t sec_wifi_cancel(void) { return esp_wifi_scan_stop(); }
