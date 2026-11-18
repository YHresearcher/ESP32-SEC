#include "lab_link.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "SEC_LAB_LINK";

bool sec_lab_request(sec_lab_action_t action) {
    const char *verb = NULL;
    switch (action) {
        case SEC_LAB_LED_ON: verb = "LED_ON"; break;
        case SEC_LAB_LED_OFF: verb = "LED_OFF"; break;
        default: return false;
    }
    /*
     * This line is consumed only by an explicitly enabled PC lab agent.
     * ESP32 cannot infer successful execution from a printf return value.
     */
    int written = printf("SEC1|CONTROL|%s\n", verb);
    fflush(stdout);
    ESP_LOGI(TAG, "lab request queued: %s (PC must opt in)", verb);
    return written > 0;
}
