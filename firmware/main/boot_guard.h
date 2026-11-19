#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/* Persistent consecutive incomplete boots. Safe mode starts after 2.
 * Does not erase the user's NVS configuration.
 */
esp_err_t sec_boot_guard_begin(bool *safe_mode, uint8_t *attempts);
esp_err_t sec_boot_guard_healthy(void);
