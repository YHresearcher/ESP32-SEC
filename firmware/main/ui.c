#include "ui.h"
#include "wifi_scan.h"
#include "ble_scan.h"
#include "feature_registry.h"
#include "security_audit.h"
#include "sec_report.h"
#include "lab_link.h"
#include "boot_guard.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"
#include "nvs.h"
#include "esp_heap_caps.h"
#include "esp_psram.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
LV_FONT_DECLARE(sec_font_vi_13);
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef enum { LANG_VI, LANG_EN } lang_t;
typedef enum { PAGE_HOME, PAGE_WIFI, PAGE_BLE, PAGE_DEFENSE,
               PAGE_ANALYTICS, PAGE_DIAG, PAGE_SETTINGS, PAGE_LAB, PAGE_MODULES, PAGE_CONTROL } page_t;
static lang_t lang = LANG_VI;
static page_t page = PAGE_HOME;
static lv_obj_t *root, *heading, *content_panel, *body, *back_btn, *start_btn, *lang_btn, *tiles[9];
static volatile bool scan_busy = false, scan_ready = false;
static volatile bool ble_busy = false, ble_ready = false;
static esp_err_t ble_result = ESP_OK;
static uint16_t ble_count = 0;
static sec_ble_device_t ble_results[SEC_MAX_BLE];
static esp_err_t scan_result = ESP_OK;
static uint16_t ap_count = 0;
static sec_wifi_ap_t results[SEC_MAX_APS];
static char report[1500];
static char diag[480];
// High-contrast palette. Explicitly style child text: LVGL button theme may
// override inherited text color and hide labels on dark surfaces.
#define SEC_TEXT_WHITE 0xFFFFFF
#define SEC_BG_PANEL 0x10283A
#define SEC_BG_BUTTON 0x19526A
static const char *menus[2][9] = {
    {"Wi-Fi", "Bluetooth BLE", "Phòng vệ", "Phân tích", "Chẩn đoán", "Cài đặt", "Pentest Lab", "Mô-đun", "Điều khiển"},
    {"Wi-Fi", "Bluetooth BLE", "Defense", "Analytics", "Diagnostics", "Settings", "Pentest Lab", "Modules", "Lab Control"},
};
static const char *page_names[2][10] = {
    {"ESP32-SEC", "Wi-Fi Audit", "Bluetooth BLE", "Phòng vệ", "Phân tích", "Chẩn đoán", "Cài đặt", "Pentest Lab", "Mô-đun", "Lab Control"},
    {"ESP32-SEC", "Wi-Fi Audit", "Bluetooth BLE", "Defense", "Analytics", "Diagnostics", "Settings", "Pentest Lab", "Modules", "Lab Control"},
};
static void persist_language(void) {
    nvs_handle_t h;
    if (nvs_open("sec_ui", NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_u8(h, "lang", (uint8_t)lang);
    nvs_commit(h);
    nvs_close(h);
}
static const char *sec_wifi_auth_name(int a) {
    switch (a) {
        case 0: return "OPEN";
        case 1: return "WEP";
        case 2: return "WPA";
        case 3: return "WPA2";
        case 4: return "MIXED";
        case 5: return "WPA2-ENT";
        case 6: return "WPA3";
        case 7: return "WPA2/WPA3";
        default: return "OTHER";
    }
}
static const char *sec_wifi_risk_label(int a) {
    if (a == 0 || a == 1 || a == 2) return "HIGH";
    if (a == 3 || a == 4 || a == 5) return "REVIEW";
    if (a == 6 || a == 7) return "LOW";
    return "UNKNOWN";
}
static void render_analytics(void) {
    if (!scan_ready || scan_result != ESP_OK) {
        lv_label_set_text(body, lang == LANG_VI ?
            "Phân tích kênh\n\nHãy chạy Wi-Fi Scan trước.\nSau đó xem kênh 1/6/11."
            : "Channel analytics\n\nRun Wi-Fi Scan first.");
        return;
    }
    sec_wifi_audit_t a;
    sec_wifi_audit(results, ap_count, &a);
    snprintf(report, sizeof(report), lang == LANG_VI ?
        "PHÂN TÍCH WI-FI 2.4GHz\n\nAP quan sát: %u\nTín hiệu mạnh: %u\nTín hiệu yếu: %u\n\nCH 01: %d AP\nCH 06: %d AP\nCH 11: %d AP\n\nKênh ít AP nhất: %d\n\nĐây là số lượng AP,\nkhông phải phép đo nhiễu RF.\nChỉ nên đổi kênh trên AP\nmà bạn sở hữu."
        : "2.4GHz CHANNEL AUDIT\n\nObserved AP: %u\nStrong RSSI: %u\nWeak RSSI: %u\n\nCH 01: %d AP\nCH 06: %d AP\nCH 11: %d AP\n\nLeast occupied: CH%d\n\nAP count only; RF noise\nwas NOT measured.\nChange only your own AP.",
        a.total, a.strong_signal, a.low_signal,
        a.channels[1], a.channels[6], a.channels[11], a.recommended_channel);
    lv_label_set_text(body, report);
}
static void render_defense(void) {
    if (!scan_ready || scan_result != ESP_OK) {
        lv_label_set_text(body, lang == LANG_VI ?
            "KIỂM TRA CẤU HÌNH\n\nQuét Wi-Fi trước để xem\ncác mạng có chuẩn yếu."
            : "CONFIGURATION AUDIT\n\nRun Wi-Fi Scan first.");
        return;
    }
    sec_wifi_audit_t a;
    sec_wifi_audit(results, ap_count, &a);
    snprintf(report, sizeof(report), lang == LANG_VI ?
        "WI-FI SECURITY AUDIT\n\nTổng AP: %u\nCấu hình rủi ro: %u\n- Open: %u\n- WEP: %u\n- WPA cũ: %u\nWPA2 trở lên*: %u\nSSID trùng: %u\nSSID ẩn: %u\n\nKHUYẾN NGHỊ\n1. Tránh WEP/WPA cũ.\n2. Dùng WPA2-AES/WPA3.\n3. Xác minh SSID trùng.\n\n*Phân loại sơ bộ.\nScan không chứng minh\nthiết bị đã bị tấn công."
        : "WI-FI SECURITY AUDIT\n\nObserved APs: %u\nWeak configuration: %u\n- Open: %u\n- WEP: %u\n- Legacy WPA: %u\nWPA2 or newer*: %u\nRepeated SSID: %u\nHidden SSID: %u\n\nRECOMMENDATIONS\n1. Avoid WEP/old WPA.\n2. Prefer WPA2-AES/WPA3.\n3. Review repeated SSID.\n\n*Coarse classification.\nA scan cannot prove attack.",
        a.total, a.high_risk, a.open, a.wep,
        a.weak_legacy, a.wpa2_or_better, a.duplicate_ssid, a.hidden);
    lv_label_set_text(body, report);
}
static void render_lab(void) {
    unsigned passed=0,total=0;
    bool ok=sec_lab_parser_selftest(&passed,&total);
    snprintf(report,sizeof(report),lang == LANG_VI ?
       "PENTEST LAB - CỤC BỘ\n\nPoC kiểm tra parser\n%u/%u bài: %s\n\nKiểm tra giới hạn bộ đệm,\nchuỗi rỗng, độ dài không hợp lệ.\n\nKhông gửi gói tin.\nKhông khai thác thiết bị khác.\n\nChưa có PoC mạng bên ngoài." :
       "LOCAL PENTEST LAB\n\nParser validation PoC\n%u/%u tests: %s\n\nChecks bounds, empty strings,\nand invalid lengths.\n\nNo packets transmitted.\nNo external exploit installed.",
       passed,total,ok ? "PASS" : "FAIL");
    lv_label_set_text(body,report);
}
static void render_modules(void) {
    lv_label_set_text(body,lang == LANG_VI ?
      "MÔ-ĐUN NGOẠI VI\n\nNRF24: Cần phần cứng\nCC1101: Cần phần cứng\nIR: Cần phần cứng\nNFC/RFID: Cần phần cứng\nGNSS: Cần phần cứng\nmicroSD: Cần phần cứng\n\nDriver chưa kích hoạt.\nCần xác nhận sơ đồ chân." :
      "EXTENSION MODULES\n\nNRF24: Hardware needed\nCC1101: Hardware needed\nIR: Hardware needed\nNFC/RFID: Hardware needed\nGNSS: Hardware needed\nmicroSD: Hardware needed\n\nDrivers not enabled.");
}
static void refresh(void);
static void back_cb(lv_event_t *e) { (void)e; page = PAGE_HOME; refresh(); }
static void lang_cb(lv_event_t *e) {
    (void)e;
    if (page == PAGE_WIFI || page == PAGE_DEFENSE || page == PAGE_ANALYTICS) {
        if (scan_ready && !scan_busy && scan_result == ESP_OK) {
            if (sec_report_wifi(results, ap_count))
                lv_label_set_text(body, lang == LANG_VI ?
                    "Đã xuất Wi-Fi JSON qua USB serial.\\nKiểm tra PC để lưu báo cáo." :
                    "Wi-Fi JSON sent to USB console.\\nSave it on the host PC.");
            else lv_label_set_text(body, "Export failed");
        }
        return;
    }
    if (page == PAGE_BLE) {
        if (ble_ready && !ble_busy && ble_result == ESP_OK) {
            if (sec_report_ble(ble_results, ble_count))
                lv_label_set_text(body, lang == LANG_VI ?
                    "Đã xuất BLE JSON qua USB serial.\\nKiểm tra PC để lưu báo cáo." :
                    "BLE JSON sent to USB console.\\nSave it on the host PC.");
            else lv_label_set_text(body, "Export failed");
        }
        return;
    }

    if (page == PAGE_CONTROL) {
        bool queued = sec_lab_request(SEC_LAB_LED_OFF);
        lv_label_set_text(body, queued ? (lang == LANG_VI ? "Đã gửi yêu cầu LED OFF.\nKiểm tra phản hồi trên PC.\nChưa xác nhận đích đã thực hiện." : "LED OFF request sent.\nCheck PC agent for confirmation.") : "Console unavailable.");
        return;
    }
    lang = lang == LANG_VI ? LANG_EN : LANG_VI;
    persist_language(); refresh();
}
static void tile_cb(lv_event_t *e) {
    page = (page_t)((uintptr_t)lv_event_get_user_data(e) + 1);
    refresh();
}
static void wifi_worker(void *unused) {
    (void)unused;
    uint16_t count = 0;
    scan_result = sec_wifi_scan(results, &count);
    ap_count = count;
    scan_ready = true;
    scan_busy = false;
    vTaskDelete(NULL);
}
static void ble_worker(void *unused) {
    (void)unused;
    uint16_t count = 0;
    ble_result = sec_ble_scan(ble_results, &count);
    ble_count = count; ble_ready = true; ble_busy = false;
    vTaskDelete(NULL);
}
static void render_ble(void) {
    if (ble_busy) {
        lv_label_set_text(body, lang == LANG_VI ? "Đang quét BLE..." : "Scanning BLE...");
        return;
    }
    if (!ble_ready) {
        lv_label_set_text(body, lang == LANG_VI ?
            "BLE DISCOVERY\n\nChạm QUÉT để tìm thiết bị.\nHiển thị RSSI, tên và\nđịa chỉ quan sát được.\n\nRSSI không phải khoảng cách.\nKhông xác minh lỗ hổng."
            : "BLE DISCOVERY\n\nTap SCAN for devices,\nRSSI and visible address.\nRSSI is NOT distance.\nNo exploit performed.");
        return;
    }
    if (ble_result != ESP_OK) {
        snprintf(report, sizeof(report), "BLE: %s", esp_err_to_name(ble_result));
        lv_label_set_text(body, report);
        return;
    }
    sec_ble_audit_t a;
    sec_ble_audit(ble_results, ble_count, &a);
    size_t used=(size_t)snprintf(report,sizeof(report),
        lang == LANG_VI ?
        "BLE SIGNAL AUDIT\n\nThiết bị: %u\nCó tên: %u | Không tên: %u\nMạnh >= -60: %u\nVừa: %u | Yếu <= -85: %u\n\n"
        : "BLE SIGNAL AUDIT\n\nDevices: %u\nNamed: %u | Unnamed: %u\nStrong >= -60: %u\nMid: %u | Weak <= -85: %u\n\n",
        a.total,a.named,a.anonymous,a.strong,a.moderate,a.weak);
    for(uint16_t i=0;i<ble_count && i<10 && used<sizeof(report)-120;i++) {
        const sec_ble_device_t *d=&ble_results[i];
        const char *tier=d->rssi>=-60?"STRONG":(d->rssi<=-85?"WEAK":"MID");
        int n=snprintf(report+used,sizeof(report)-used,
            "%.28s\n%d dBm | %s\n%s\n\n",
            d->name[0]?d->name:"(unnamed)",(int)d->rssi,tier,d->address);
        if(n<=0 || (size_t)n>=sizeof(report)-used)break;
        used+=(size_t)n;
    }
    if (used < sizeof(report)-80) {
        snprintf(report+used,sizeof(report)-used,
            "RSSI != distance.\nScan != vulnerability proof.");
    }
    lv_label_set_text(body, report);
}
static void start_cb(lv_event_t *e) {
    (void)e;
    if (page == PAGE_CONTROL) {
        bool queued = sec_lab_request(SEC_LAB_LED_ON);
        lv_label_set_text(body, queued ? (lang == LANG_VI ? "Đã gửi yêu cầu LED ON.\nKiểm tra phản hồi trên PC.\nChưa xác nhận đích đã thực hiện." : "LED ON request sent.\nCheck PC agent for confirmation.") : "Console unavailable.");
        return;
    }
    if (page == PAGE_WIFI && scan_busy) { sec_wifi_cancel(); lv_label_set_text(body,"Stopping Wi-Fi scan..."); return; }
    if (page == PAGE_BLE && ble_busy) { sec_ble_cancel(); lv_label_set_text(body,"Stopping BLE scan..."); return; }
    if ((page != PAGE_WIFI && page != PAGE_BLE) || scan_busy || ble_busy) return;
    if (page == PAGE_BLE) {
        ble_ready=false; ble_busy=true;
        if (xTaskCreate(ble_worker,"ble_scan",6144,NULL,4,NULL)!=pdPASS) { ble_busy=false; lv_label_set_text(body,"BLE task start failed"); }
        else lv_label_set_text(body, "Scanning BLE...");
        return;
    }
    scan_ready = false;
    scan_busy = true;
    if (xTaskCreate(wifi_worker, "wifi_scan", 6144, NULL, 4, NULL) != pdPASS) {
        scan_busy = false;
        lv_label_set_text(body, "Could not start scan task.");
    } else lv_label_set_text(body, lang == LANG_VI ? "Đang quét..." : "Scanning...");
}
static void render_wifi(void) {
    if (scan_busy) {
        lv_label_set_text(body, lang == LANG_VI ? "Đang quét Wi-Fi..." : "Scanning Wi-Fi...");
        return;
    }
    if (!scan_ready) {
        lv_label_set_text(body, lang == LANG_VI ?
            "WI-FI SECURITY\n\nChạm QUÉT để kiểm tra.\n\nSau khi quét, chọn:\n- Phòng vệ: đánh giá mã hóa\n- Phân tích: mật độ kênh\n\nChỉ đo thụ động/cơ bản."
            : "WI-FI SECURITY\n\nTap SCAN.\nThen open Defense for\nsecurity findings or\nAnalytics for channels.");
        return;
    }
    if (scan_result != ESP_OK) {
        snprintf(report, sizeof(report), "Wi-Fi: %s", esp_err_to_name(scan_result));
        lv_label_set_text(body, report);
        return;
    }
    sec_wifi_audit_t audit;
    sec_wifi_audit(results, ap_count, &audit);
    size_t used = (size_t)snprintf(report, sizeof(report),
        lang == LANG_VI ?
        "WI-FI AUDIT | %u AP\nRủi ro cấu hình: %u\nKênh đề xuất*: %d\n\n"
        : "WI-FI AUDIT | %u AP\nWeak config: %u\nSuggested channel*: %d\n\n",
        audit.total, audit.high_risk, audit.recommended_channel);
    for (uint16_t i=0; i<ap_count && i<10 && used<sizeof(report)-120; i++) {
        const sec_wifi_ap_t *ap = &results[i];
        int n = snprintf(report+used, sizeof(report)-used,
            "%u. %.30s\nCH%u %d dBm | %s\nRisk: %s\n\n",
            (unsigned)(i+1), ap->ssid[0]?ap->ssid:"(hidden)",
            (unsigned)ap->channel, (int)ap->rssi,
            sec_wifi_auth_name(ap->authmode), sec_wifi_risk_label(ap->authmode));
        if(n<=0 || (size_t)n>=sizeof(report)-used)break;
        used += (size_t)n;
    }
    if (used < sizeof(report)-80) {
        snprintf(report+used, sizeof(report)-used,
            "*Least AP count CH1/6/11\nNot RF noise. Audit only.");
    }
    lv_label_set_text(body, report);
}
static void refresh(void) {
    lv_label_set_text(heading, page_names[lang][page]);
    bool home = page == PAGE_HOME;
    for (int i=0;i<9;i++) {
        if (!tiles[i]) continue;
        if (home) lv_obj_remove_flag(tiles[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(tiles[i], LV_OBJ_FLAG_HIDDEN);
        if (tiles[i]) lv_label_set_text(lv_obj_get_child(tiles[i],0), menus[lang][i]);
    }
    if (home) {
        lv_obj_add_flag(content_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(back_btn, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(content_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(back_btn, LV_OBJ_FLAG_HIDDEN);
    }
    if (page == PAGE_WIFI || page == PAGE_BLE || page == PAGE_CONTROL) lv_obj_remove_flag(start_btn, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(start_btn, LV_OBJ_FLAG_HIDDEN);
    if (page == PAGE_SETTINGS || page == PAGE_CONTROL || ((page == PAGE_WIFI || page == PAGE_DEFENSE || page == PAGE_ANALYTICS) && scan_ready && !scan_busy && scan_result == ESP_OK) || (page == PAGE_BLE && ble_ready && !ble_busy && ble_result == ESP_OK)) lv_obj_remove_flag(lang_btn, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(lang_btn, LV_OBJ_FLAG_HIDDEN);
    // Export and scan share the footer only after a completed scan.
    // Move scan to center so buttons cannot overlap at 240x320.
    bool report_export_visible = (page == PAGE_WIFI && scan_ready && !scan_busy && scan_result == ESP_OK) ||
                                 (page == PAGE_BLE && ble_ready && !ble_busy && ble_result == ESP_OK);
    if (report_export_visible) lv_obj_align(start_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    else lv_obj_align(start_btn, LV_ALIGN_BOTTOM_RIGHT, -9, -10);
    lv_label_set_text(lv_obj_get_child(lang_btn,0), page == PAGE_CONTROL ? "LED OFF" : (page == PAGE_SETTINGS ? (lang == LANG_VI ? "EN" : "VI") : (lang == LANG_VI ? "Xuất JSON" : "JSON")));
    lv_label_set_text(lv_obj_get_child(start_btn,0), page == PAGE_CONTROL ? "LED ON" : (((page==PAGE_WIFI && scan_busy) || (page==PAGE_BLE && ble_busy)) ? (lang == LANG_VI ? "Dừng" : "Stop") : (lang == LANG_VI ? "Quét" : "Scan")));
    if (page == PAGE_WIFI) render_wifi();
    if (page == PAGE_BLE) render_ble();
    if (page == PAGE_DEFENSE) render_defense();
    if (page == PAGE_ANALYTICS) render_analytics();
    if (page == PAGE_LAB) render_lab();
    if (page == PAGE_MODULES) render_modules();
    if (page == PAGE_CONTROL) lv_label_set_text(body, lang == LANG_VI ?
        "LAB CONTROLLER\n\nKết nối ESP32-SEC qua USB\nvới PC chạy lab_controller.py\n\nChỉ 2 hành động cho phép:\nLED ON / LED OFF.\n\nPhải bật --enable-control\ntrên PC trước khi thử.\nKhông có remote shell.\n\nMọi yêu cầu chỉ gửi đi,\nchưa xác nhận thực hiện." :
        "LAB CONTROLLER\n\nConnect ESP32-SEC over USB\nto lab_controller.py on PC.\n\nFixed LED ON/OFF actions only.\nHost requires --enable-control.\nNo remote shell.\nRequests are not acknowledgments.");
    if (page == PAGE_DIAG) {
        size_t feature_total = 0;
        sec_feature_catalog(&feature_total);
        snprintf(diag, sizeof(diag),
            "ESP32-S3 N16R8\nPSRAM: %u KB\nFree heap: %u KB\nLCD: ST7789V 240x320\nTouch: XPT2046\nBL: not controlled\nRegistry: %u built / %u planned",
            (unsigned)(esp_psram_get_size()/1024), (unsigned)(esp_get_free_heap_size()/1024),
            (unsigned)sec_feature_compiled_count(), (unsigned)feature_total);
        lv_label_set_text(body, diag);
    }
    if (page == PAGE_SETTINGS) lv_label_set_text(body, lang == LANG_VI ?
        "Ngôn ngữ: Tiếng Việt\nChuyển sang English bằng EN." :
        "Language: EN\nSwitch to Vietnamese with VI.\n\nVietnamese font pending.");
}
static void ui_timer(lv_timer_t *timer) {
    (void)timer;
    static unsigned ticks = 0;
    // Mark a boot healthy only after the LVGL task has executed several ticks.
    if (ticks < 4 && ++ticks == 4) {
        esp_err_t status = sec_boot_guard_healthy();
        if (status != ESP_OK) ESP_LOGW("SEC_UI", "boot guard clear failed: %s", esp_err_to_name(status));
    }
    static bool prev_wifi_busy = false, prev_ble_busy = false;
    if (prev_wifi_busy != scan_busy || prev_ble_busy != ble_busy) {
        prev_wifi_busy = scan_busy; prev_ble_busy = ble_busy;
        if (page == PAGE_WIFI || page == PAGE_BLE) refresh();
    }
}
void sec_ui_init(bool recovery_mode) {
    if (recovery_mode) {
        if (!lvgl_port_lock(1000)) {
            ESP_LOGE("SEC_UI", "Recovery LVGL mutex timeout");
            return;
        }
        root = lv_screen_active();
        lv_obj_set_style_bg_color(root, lv_color_hex(0x092033), 0);
        lv_obj_set_style_text_color(root, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(root, &lv_font_montserrat_16, 0);
        lv_obj_t *recovery_label = lv_label_create(root);
        lv_obj_set_style_text_color(recovery_label, lv_color_hex(SEC_TEXT_WHITE), 0);
        lv_obj_set_width(recovery_label, 215);
        lv_label_set_text(recovery_label,
            "ESP32-SEC RECOVERY\n\nRepeated incomplete boots\nWi-Fi / BLE / touch OFF\n\nUSB serial: 115200\nCheck crash logs.\n\nRestart to retry normal UI.");
        lv_label_set_long_mode(recovery_label, LV_LABEL_LONG_WRAP);
        lv_obj_center(recovery_label);
        lv_timer_create(ui_timer, 1000, NULL);
        ESP_LOGW("SEC_UI", "RECOVERY_UI_READY");
        lvgl_port_unlock();
        return;
    }
    nvs_handle_t h;
    if (nvs_open("sec_ui", NVS_READONLY, &h) == ESP_OK) {
        uint8_t n = 0;
        if (nvs_get_u8(h, "lang", &n) == ESP_OK && n <= 1) lang = (lang_t)n;
        nvs_close(h);
    }
    if (!lvgl_port_lock(1000)) { ESP_LOGE("SEC_UI", "LVGL mutex timeout"); return; }
    root = lv_screen_active();
    lv_obj_set_style_bg_color(root, lv_color_hex(0x0B1728), 0);
    lv_obj_set_style_text_color(root, lv_color_hex(SEC_TEXT_WHITE), 0);
    lv_obj_set_style_text_font(root, &sec_font_vi_13, 0);
    heading = lv_label_create(root);
    lv_obj_set_style_text_font(heading, &sec_font_vi_13, 0);
    lv_obj_set_style_text_color(heading, lv_color_hex(SEC_TEXT_WHITE), 0);
    lv_obj_align(heading, LV_ALIGN_TOP_MID, 0, 9);
    for (int i=0;i<9;i++) {
        tiles[i] = lv_button_create(root);
        if (!tiles[i]) { ESP_LOGE("SEC_UI", "Tile %d allocation failed", i); continue; }
        lv_obj_set_size(tiles[i], 106, 40);
        lv_obj_set_pos(tiles[i], 10+(i%2)*114, 39+(i/2)*44);
        lv_obj_set_style_bg_color(tiles[i], lv_color_hex(SEC_BG_BUTTON), 0);
        lv_obj_set_style_text_color(tiles[i], lv_color_hex(SEC_TEXT_WHITE), 0);
        lv_obj_set_style_border_width(tiles[i], 1, 0);
        lv_obj_set_style_border_color(tiles[i], lv_color_hex(0x21A9B1), 0);
        lv_obj_set_style_radius(tiles[i], 9, 0);
        lv_obj_add_event_cb(tiles[i], tile_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
        lv_obj_t *label = lv_label_create(tiles[i]);
        lv_obj_set_style_text_color(label, lv_color_hex(SEC_TEXT_WHITE), 0);
        lv_obj_set_width(label, 100);
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(label);
    }
    content_panel = lv_obj_create(root);
    lv_obj_set_pos(content_panel, 8, 43);
    lv_obj_set_size(content_panel, 224, 222);
    lv_obj_set_style_bg_color(content_panel, lv_color_hex(SEC_BG_PANEL), 0);
    lv_obj_set_style_text_color(content_panel, lv_color_hex(SEC_TEXT_WHITE), 0);
    lv_obj_set_style_border_color(content_panel, lv_color_hex(0x236D7A), 0);
    lv_obj_set_style_border_width(content_panel, 1, 0);
    lv_obj_set_style_radius(content_panel, 8, 0);
    lv_obj_set_style_pad_all(content_panel, 8, 0);
    lv_obj_add_flag(content_panel, LV_OBJ_FLAG_SCROLLABLE);
    body = lv_label_create(content_panel);
    lv_obj_set_style_text_color(body, lv_color_hex(SEC_TEXT_WHITE), 0);
    lv_obj_set_style_text_font(body, &sec_font_vi_13, 0);
    lv_obj_set_width(body, 202);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_obj_align(body, LV_ALIGN_TOP_LEFT, 0, 0);
    back_btn = lv_button_create(root);
    lv_obj_set_size(back_btn, 86, 37);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 9, -10);
    lv_obj_add_event_cb(back_btn, back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(SEC_BG_BUTTON), 0);
    lv_obj_set_style_text_color(back_btn, lv_color_hex(SEC_TEXT_WHITE), 0);
    lv_obj_t *bl = lv_label_create(back_btn);
    lv_obj_set_style_text_color(bl, lv_color_hex(SEC_TEXT_WHITE), 0);
    lv_label_set_text(bl, "Trở về"); lv_obj_center(bl);
    start_btn = lv_button_create(root);
    lv_obj_set_size(start_btn, 86, 37);
    lv_obj_align(start_btn, LV_ALIGN_BOTTOM_RIGHT, -9, -10);
    lv_obj_add_event_cb(start_btn, start_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(start_btn, lv_color_hex(SEC_BG_BUTTON), 0);
    lv_obj_set_style_text_color(start_btn, lv_color_hex(SEC_TEXT_WHITE), 0);
    lv_obj_t *sl = lv_label_create(start_btn);
    lv_obj_set_style_text_color(sl, lv_color_hex(SEC_TEXT_WHITE), 0);
    lv_label_set_text(sl, "Quét"); lv_obj_center(sl);
    lang_btn = lv_button_create(root);
    lv_obj_set_size(lang_btn, 86, 37);
    lv_obj_align(lang_btn, LV_ALIGN_BOTTOM_RIGHT, -9, -10);
    lv_obj_add_event_cb(lang_btn, lang_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(lang_btn, lv_color_hex(SEC_BG_BUTTON), 0);
    lv_obj_set_style_text_color(lang_btn, lv_color_hex(SEC_TEXT_WHITE), 0);
    lv_obj_t *ll = lv_label_create(lang_btn);
    lv_obj_set_style_text_color(ll, lv_color_hex(SEC_TEXT_WHITE), 0);
    lv_obj_center(ll);
    lv_timer_create(ui_timer, 1000, NULL);
    refresh();
    lvgl_port_unlock();
}
