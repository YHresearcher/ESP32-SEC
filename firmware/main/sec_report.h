#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "wifi_scan.h"
#include "ble_scan.h"

// Explicit operator-triggered, USB-console-only audit reports.
// No network upload, persistent storage or credentials.
bool sec_report_wifi(const sec_wifi_ap_t *aps, uint16_t count);
bool sec_report_ble(const sec_ble_device_t *devices, uint16_t count);
