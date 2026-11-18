#pragma once
#include <stdbool.h>

/* Manual, local-only protocol over the configured ESP-IDF console.
 * Host MUST explicitly opt in. Only fixed LED control verbs are supported.
 * Emits a request, NOT confirmation that the target has responded.
 */
typedef enum {
    SEC_LAB_LED_ON = 0,
    SEC_LAB_LED_OFF
} sec_lab_action_t;

bool sec_lab_request(sec_lab_action_t action);
