#include "security_audit.h"
#include <string.h>
#include <limits.h>

void sec_wifi_audit(const sec_wifi_ap_t *aps, size_t count, sec_wifi_audit_t *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (!aps) return;
    out->total = (unsigned)count;
    out->signal_best = -127;
    for (size_t i = 0; i < count; ++i) {
        const sec_wifi_ap_t *p = &aps[i];
        if (p->authmode == 0) ++out->open;  // WIFI_AUTH_OPEN
        if (p->authmode == 1) ++out->wep;   // WIFI_AUTH_WEP
        if (p->authmode == 2) ++out->weak_legacy; // WPA PSK legacy
        if (p->authmode == 0 || p->authmode == 1 || p->authmode == 2)
            ++out->high_risk; // Configuration only; not proof of exploitation.
        if (p->authmode >= 3) ++out->wpa2_or_better;
        if (p->rssi >= -60) ++out->strong_signal;
        if (p->rssi <= -85) ++out->low_signal;
        if (!p->ssid[0]) ++out->hidden;
        if (p->channel >= 1 && p->channel <= 14) out->channels[p->channel]++;
        if (p->rssi > out->signal_best) out->signal_best = p->rssi;
        if (p->ssid[0]) {
            for (size_t j=0; j<i; ++j) {
                if (strcmp(p->ssid, aps[j].ssid)==0 &&
                    strcmp(p->bssid, aps[j].bssid)!=0) {
                    ++out->duplicate_ssid;
                    break;
                }
            }
        }
    }
    // Only recommend standard non-overlapping 2.4 GHz channels
    // based on AP counts, NOT a measured RF interference score.
    const int candidates[] = {1, 6, 11};
    int best = INT_MAX;
    for (size_t i=0; i<3; ++i) {
        int ch = candidates[i];
        int density=out->channels[ch];
        if (density < best) { best = density; out->recommended_channel = ch; }
    }
}

void sec_ble_audit(const sec_ble_device_t *devices, size_t count, sec_ble_audit_t *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (!devices) return;
    out->total = (unsigned)count;
    for (size_t i=0; i<count; ++i) {
        const char *name = devices[i].name;
        if (name[0] && strcmp(name, "<unnamed>") != 0) ++out->named;
        else ++out->anonymous;
        if (devices[i].rssi >= -60) ++out->strong;
        if (devices[i].rssi <= -85) ++out->weak;
        if (devices[i].rssi > -85 && devices[i].rssi < -60) ++out->moderate;
    }
}

static bool bounded_copy(char *dst, size_t capacity, const uint8_t *src, size_t length) {
    if (!dst || !capacity || (!src && length)) return false;
    if (length >= capacity) return false;
    if (length) memcpy(dst, src, length);
    dst[length] = '\0';
    return true;
}

bool sec_lab_parser_selftest(unsigned *passed, unsigned *total) {
    unsigned ok = 0, n=0;
    char dst[8]; uint8_t valid[7] = {'S','E','C','-','T','E','S'};
    memset(dst, 0xA5, sizeof(dst));
    ++n; if (bounded_copy(dst,sizeof(dst),valid,7) && dst[7]==0) ++ok;
    ++n; if (!bounded_copy(dst,sizeof(dst),valid,8)) ++ok;
    ++n; if (!bounded_copy(dst,sizeof(dst),NULL,1)) ++ok;
    ++n; if (bounded_copy(dst,sizeof(dst),valid,0) && dst[0]==0) ++ok;
    ++n; if (!bounded_copy(dst,0,valid,1)) ++ok;
    if (passed) *passed = ok;
    if (total) *total = n;
    return ok == n;
}
