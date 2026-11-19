#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "wifi_scan.h"
#include "ble_scan.h"

typedef struct {
    unsigned total, open, wep, weak_legacy, duplicate_ssid, hidden;
    unsigned wpa2_or_better, high_risk, strong_signal, low_signal;
    int channels[15];
    int recommended_channel;
    int signal_best;
} sec_wifi_audit_t;
void sec_wifi_audit(const sec_wifi_ap_t *aps, size_t count, sec_wifi_audit_t *out);
typedef struct {
    unsigned total, named, anonymous, strong, weak;
    unsigned moderate, address_randomized_unknown;
} sec_ble_audit_t;
void sec_ble_audit(const sec_ble_device_t *devices, size_t count, sec_ble_audit_t *out);
// Local-only PoC: test strict bounded copying against hostile-length frames.
// Does not transmit packets or interact with external devices.
bool sec_lab_parser_selftest(unsigned *cases_passed, unsigned *cases_total);
