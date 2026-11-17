#pragma once
#include <stdint.h>
#include "esp_err.h"
typedef struct {
    char ssid[33];
    char bssid[18];
    int8_t rssi;
    uint8_t channel;
    int authmode;
} sec_wifi_ap_t;
#define SEC_MAX_APS 30
esp_err_t sec_wifi_init(void);
// Blocking scan: call only from background FreeRTOS worker; NEVER from LVGL event.
esp_err_t sec_wifi_scan(sec_wifi_ap_t *out, uint16_t *count);
