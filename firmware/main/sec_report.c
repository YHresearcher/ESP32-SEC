#include "sec_report.h"
#include "security_audit.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "SEC_REPORT";

/* Console JSON strings must not contain raw control characters or quotes.
 * Each station's metadata is read only from bounded scan records. */
static void print_json_string(const char *src, size_t max_len) {
    putchar('"');
    if (src) {
        for (size_t i = 0; i < max_len && src[i]; ++i) {
            unsigned char c = (unsigned char)src[i];
            if (c == '"' || c == '\\') {
                putchar('\\'); putchar(c);
            } else if (c < 0x20 || c >= 0x7f) {
                // Escaped bytes keep the wire format ASCII and valid JSON,
                // including SSIDs that do not encode valid UTF-8.
                printf("\\u%04x", (unsigned)c);
            } else putchar(c);
        }
    }
    putchar('"');
}

bool sec_report_wifi(const sec_wifi_ap_t *aps, uint16_t count) {
    if ((!aps && count) || count > SEC_MAX_APS) return false;
    sec_wifi_audit_t a;
    sec_wifi_audit(aps, count, &a);
    printf("SEC1|REPORT|{\"type\":\"wifi\",\"count\":%u,\"weak_config\":%u,"
           "\"open\":%u,\"wep\":%u,\"wpa_legacy\":%u,\"channels\":[",
           a.total, a.high_risk, a.open, a.wep, a.weak_legacy);
    for (int ch = 1; ch <= 14; ++ch) printf("%s%d", ch == 1 ? "" : ",", a.channels[ch]);
    printf("],\"aps\":[");
    for (uint16_t i = 0; i < count; ++i) {
        const sec_wifi_ap_t *p = &aps[i];
        if (i) putchar(',');
        printf("{\"ssid\":");
        print_json_string(p->ssid, sizeof(p->ssid));
        printf(",\"bssid\":");
        print_json_string(p->bssid, sizeof(p->bssid));
        printf(",\"channel\":%u,\"rssi\":%d,\"authmode\":%d}",
               (unsigned)p->channel, (int)p->rssi, p->authmode);
    }
    printf("]}\n");
    fflush(stdout);
    ESP_LOGI(TAG, "Wi-Fi report emitted on console: %u AP", (unsigned)count);
    return true;
}

bool sec_report_ble(const sec_ble_device_t *devices, uint16_t count) {
    if ((!devices && count) || count > SEC_MAX_BLE) return false;
    sec_ble_audit_t a;
    sec_ble_audit(devices, count, &a);
    printf("SEC1|REPORT|{\"type\":\"ble\",\"count\":%u,\"named\":%u,"
           "\"strong\":%u,\"moderate\":%u,\"weak\":%u,\"devices\":[",
           a.total, a.named, a.strong, a.moderate, a.weak);
    for (uint16_t i = 0; i < count; ++i) {
        if (i) putchar(',');
        printf("{\"address\":");
        print_json_string(devices[i].address, sizeof(devices[i].address));
        printf(",\"name\":");
        print_json_string(devices[i].name, sizeof(devices[i].name));
        printf(",\"rssi\":%d}", (int)devices[i].rssi);
    }
    printf("]}\n");
    fflush(stdout);
    ESP_LOGI(TAG, "BLE report emitted on console: %u devices", (unsigned)count);
    return true;
}
