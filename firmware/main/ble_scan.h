#pragma once
#include <stdint.h>
#include "esp_err.h"
#define SEC_MAX_BLE 24
typedef struct {
    char address[18];
    char name[32];
    int8_t rssi;
} sec_ble_device_t;
// Initializes NimBLE host (observer mode only). Returns error when unavailable.
esp_err_t sec_ble_init(void);
// Caller must invoke from its own FreeRTOS task; 5-second passive scan.
esp_err_t sec_ble_scan(sec_ble_device_t *out, uint16_t *count);

// Request early termination of a passive BLE scan.
void sec_ble_cancel(void);
