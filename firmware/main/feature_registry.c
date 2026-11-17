#include "feature_registry.h"

static const sec_feature_info_t features[] = {
    {"WIFI_SCAN", "Wi-Fi Scanner", SEC_REQUIRE_CORE, true},
    {"WIFI_PACKET", "Wi-Fi Packet Monitor", SEC_REQUIRE_SOFTWARE, false},
    {"WIFI_DEAUTH_DETECT", "Deauth Detector", SEC_REQUIRE_SOFTWARE, false},
    {"WIFI_WPS", "WPS Scanner", SEC_REQUIRE_SOFTWARE, false},
    {"WIFI_ARP", "ARP Scanner", SEC_REQUIRE_SOFTWARE, false},
    {"WIFI_LOCAL_PORTAL", "Local Provisioning Portal", SEC_REQUIRE_SOFTWARE, false},
    {"BLE_SCAN", "BLE Scanner", SEC_REQUIRE_CORE, true},
    {"BLE_MONITOR", "BLE Advertisement Monitor", SEC_REQUIRE_SOFTWARE, false},
    {"BLE_AIRTAG", "AirTag Advertisement Identifier", SEC_REQUIRE_SOFTWARE, false},
    {"BLE_SKIMMER", "BLE Anomaly Heuristics", SEC_REQUIRE_SOFTWARE, false},
    {"NRF24_SPECTRUM", "NRF24 Spectrum Monitor", SEC_REQUIRE_NRF24, false},
    {"NRF24_ESB", "ESB Receive Monitor", SEC_REQUIRE_NRF24, false},
    {"NRF24_MOUSE_SCAN", "Wireless Peripheral Safety Scan", SEC_REQUIRE_NRF24, false},
    {"SUBGHZ_CAPTURE", "Sub-GHz Receive Profiles", SEC_REQUIRE_CC1101, false},
    {"SUBGHZ_JAM_DETECT", "Sub-GHz Interference Detector", SEC_REQUIRE_CC1101, false},
    {"IR_CAPTURE", "IR Receiver", SEC_REQUIRE_IR_RX, false},
    {"IR_PROFILES", "IR Profiles", SEC_REQUIRE_IR_RX, false},
    {"IR_REMOTE", "IR Universal Remote", SEC_REQUIRE_IR_TX, false},
    {"NFC_READ", "NFC Card Identification", SEC_REQUIRE_NFC, false},
    {"NFC_DUMP", "Owned-Tag Memory Read", SEC_REQUIRE_NFC, false},
    {"NFC_ACCESS", "NFC Access Bit Decoder", SEC_REQUIRE_NFC, false},
    {"GPS_WARDRIVE", "GPS Survey Logger", SEC_REQUIRE_GNSS, false},
    {"GPS_SATELLITES", "GNSS Satellite View", SEC_REQUIRE_GNSS, false},
    {"SYS_SERIAL", "Serial Monitor", SEC_REQUIRE_SOFTWARE, false},
    {"SYS_SD", "microSD Manager", SEC_REQUIRE_SD, false},
    {"SYS_SD_UPDATE", "Firmware Update from SD", SEC_REQUIRE_SD, false},
    {"SYS_TOUCH_CAL", "Touch Calibration", SEC_REQUIRE_SOFTWARE, false},
    {"SYS_SETTINGS", "Device Settings", SEC_REQUIRE_CORE, true},
};

const sec_feature_info_t *sec_feature_catalog(size_t *count) {
    if (count) *count = sizeof(features) / sizeof(features[0]);
    return features;
}
size_t sec_feature_compiled_count(void) {
    size_t result = 0;
    for (size_t i = 0; i < sizeof(features) / sizeof(features[0]); ++i) {
        if (features[i].driver_integrated) ++result;
    }
    return result;
}
