#include "ui.h"
#include "wifi_scan.h"
#include "ble_scan.h"
#include "feature_registry.h"
#include "security_audit.h"
#include "lab_link.h"
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
static void render_analytics(void) {
    if (!scan_ready || scan_result != ESP_OK) {
        lv_label_set_text(body, lang == LANG_VI ? "Quét Wi-Fi để phân tích mật độ kênh." : "Scan Wi-Fi to analyze channel density.");
        return;
    }
    sec_wifi_audit_t a;
    sec_wifi_audit(results, ap_count, &a);
    snprintf(report, sizeof(report),
        lang == LANG_VI ?
        "PHÂN TÍCH 2.4 GHz\nAP quan sát: %u\nKênh ít AP nhất: %d\n(Chưa đo nhiễu thực tế)\n\nCH01: %d AP\nCH06: %d AP\nCH11: %d AP\n\nRSSI mạnh nhất: %d dBm" :
        "2.4 GHz ANALYTICS\nObserved APs: %u\nLeast occupied: CH%d\n(No RF interference measurement)\n\nCH01: %d AP\nCH06: %d AP\nCH11: %d AP\n\nStrongest: %d dBm",
        a.total, a.recommended_channel, a.channels[1], a.channels[6],
        a.channels[11], a.signal_best);
    lv_label_set_text(body, report);
}
static void render_defense(void) {
    if (!scan_ready || scan_result != ESP_OK) {
        lv_label_set_text(body, lang == LANG_VI ? "Quét Wi-Fi để đánh giá bảo mật." : "Run Wi-Fi scan before audit.");
        return;
    }
    sec_wifi_audit_t a;
    sec_wifi_audit(results, ap_count, &a);
    snprintf(report, sizeof(report),
        lang == LANG_VI ?
        "ĐÁNH GIÁ CẤU HÌNH\n\nAP: %u\nMạng mở: %u\nWEP: %u\nWPA cũ: %u\nSSID trùng: %u\nẨn tên: %u\n\nKhuyến nghị: dùng WPA2/WPA3.\nMạng mở không mã hóa.\nSSID trùng không chứng minh tấn công." :
        "CONFIGURATION AUDIT\n\nAPs: %u\nOpen: %u\nWEP: %u\nLegacy WPA: %u\nRepeated SSID: %u\nHidden: %u\n\nPrefer WPA2/WPA3.\nRepeated SSID does not prove an attack.",
        a.total,a.open,a.wep,a.weak_legacy,a.duplicate_ssid,a.hidden);
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
            "Chạm Quét để tìm BLE.\nKết quả sẽ có phân tích tín hiệu." :
            "Tap Scan to discover BLE devices.");
        return;
    }
    if (ble_result != ESP_OK) {
        snprintf(report,sizeof(report),"BLE: %s",esp_err_to_name(ble_result));
        lv_label_set_text(body,report);
        return;
    }
    sec_ble_audit_t a;
    sec_ble_audit(ble_results,ble_count,&a);
    size_t used=(size_t)snprintf(report,sizeof(report),
        lang == LANG_VI ?
        "BLE AUDIT\nThiết bị: %u\nCó tên: %u\nẨn tên: %u\nGần (RSSI >= -60): %u\nYếu (<= -85): %u\n\n" :
        "BLE AUDIT\nDevices: %u\nNamed: %u\nUnnamed: %u\nStrong (>= -60): %u\nWeak (<= -85): %u\n\n",
        a.total,a.named,a.anonymous,a.strong,a.weak);
    for(uint16_t i=0;i<ble_count && i<12 && used<sizeof(report)-95;i++) {
        int n=snprintf(report+used,sizeof(report)-used,"%s\n%d dBm  %s\n",
            ble_results[i].name,ble_results[i].rssi,ble_results[i].address);
        if(n<=0 || (size_t)n>=sizeof(report)-used) break;
        used+=(size_t)n;
    }
    lv_label_set_text(body,report);
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
        lv_label_set_text(body, lang == LANG_VI ? "Chạm Quét để tìm mạng." : "Tap Scan to discover APs.");
        return;
    }
    if (scan_result != ESP_OK) {
        snprintf(report, sizeof(report), "Wi-Fi scan error: %s", esp_err_to_name(scan_result));
        lv_label_set_text(body, report);
        return;
    }
    size_t used = (size_t)snprintf(report, sizeof(report), "%u APs found\n\n", ap_count);
    for (uint16_t i = 0; i < ap_count && i < 12 && used < sizeof(report)-70; i++) {
        const sec_wifi_ap_t *r = &results[i];
        int n = snprintf(report + used, sizeof(report) - used,
            "%s\nCh %u  %d dBm  Auth %d\n", r->ssid[0] ? r->ssid : "<hidden>",
            r->channel, r->rssi, r->authmode);
        if (n <= 0 || (size_t)n >= sizeof(report)-used) break;
        used += (size_t)n;
    }
    lv_label_set_text(body, report);
}
static void refresh(void) {
    lv_label_set_text(heading, page_names[lang][page]);
    bool home = page == PAGE_HOME;
    for (int i=0;i<9;i++) {
        if (home) lv_obj_remove_flag(tiles[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(tiles[i], LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(lv_obj_get_child(tiles[i],0), menus[lang][i]);
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
    if (page == PAGE_SETTINGS || page == PAGE_CONTROL) lv_obj_remove_flag(lang_btn, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(lang_btn, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(lv_obj_get_child(lang_btn,0), page == PAGE_CONTROL ? "LED OFF" : (lang == LANG_VI ? "EN" : "VI"));
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
    static bool prev_wifi_busy = false, prev_ble_busy = false;
    if (prev_wifi_busy != scan_busy || prev_ble_busy != ble_busy) {
        prev_wifi_busy = scan_busy; prev_ble_busy = ble_busy;
        if (page == PAGE_WIFI || page == PAGE_BLE) refresh();
    }
}
void sec_ui_init(void) {
    nvs_handle_t h;
    if (nvs_open("sec_ui", NVS_READONLY, &h) == ESP_OK) {
        uint8_t n = 0;
        if (nvs_get_u8(h, "lang", &n) == ESP_OK && n <= 1) lang = (lang_t)n;
        nvs_close(h);
    }
    if (!lvgl_port_lock(1000)) { ESP_LOGE("SEC_UI", "LVGL mutex timeout"); return; }
    root = lv_screen_active();
    lv_obj_set_style_bg_color(root, lv_color_hex(0x0B1728), 0);
    lv_obj_set_style_text_color(root, lv_color_hex(0xF1F6FE), 0);
    lv_obj_set_style_text_font(root, &sec_font_vi_13, 0);
    heading = lv_label_create(root);
    lv_obj_set_style_text_font(heading, &sec_font_vi_13, 0);
    lv_obj_set_style_text_color(heading, lv_color_hex(0x4BE1D9), 0);
    lv_obj_align(heading, LV_ALIGN_TOP_MID, 0, 9);
    for (int i=0;i<8;i++) {
        tiles[i] = lv_button_create(root);
        lv_obj_set_size(tiles[i], 106, 48);
        lv_obj_set_pos(tiles[i], 10+(i%2)*114, 40+(i/2)*54);
        lv_obj_set_style_bg_color(tiles[i], lv_color_hex(0x14364C), 0);
        lv_obj_set_style_border_width(tiles[i], 1, 0);
        lv_obj_set_style_border_color(tiles[i], lv_color_hex(0x21A9B1), 0);
        lv_obj_set_style_radius(tiles[i], 9, 0);
        lv_obj_add_event_cb(tiles[i], tile_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
        lv_obj_t *label = lv_label_create(tiles[i]);
        lv_obj_center(label);
    }
    content_panel = lv_obj_create(root);
    lv_obj_set_pos(content_panel, 8, 43);
    lv_obj_set_size(content_panel, 224, 222);
    lv_obj_set_style_bg_color(content_panel, lv_color_hex(0x0E2335), 0);
    lv_obj_set_style_border_color(content_panel, lv_color_hex(0x236D7A), 0);
    lv_obj_set_style_border_width(content_panel, 1, 0);
    lv_obj_set_style_radius(content_panel, 8, 0);
    lv_obj_set_style_pad_all(content_panel, 8, 0);
    lv_obj_add_flag(content_panel, LV_OBJ_FLAG_SCROLLABLE);
    body = lv_label_create(content_panel);
    lv_obj_set_width(body, 202);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_obj_align(body, LV_ALIGN_TOP_LEFT, 0, 0);
    back_btn = lv_button_create(root);
    lv_obj_set_size(back_btn, 86, 37);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 9, -10);
    lv_obj_add_event_cb(back_btn, back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(back_btn);
    lv_label_set_text(bl, "Trở về"); lv_obj_center(bl);
    start_btn = lv_button_create(root);
    lv_obj_set_size(start_btn, 86, 37);
    lv_obj_align(start_btn, LV_ALIGN_BOTTOM_RIGHT, -9, -10);
    lv_obj_add_event_cb(start_btn, start_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *sl = lv_label_create(start_btn);
    lv_label_set_text(sl, "Quét"); lv_obj_center(sl);
    lang_btn = lv_button_create(root);
    lv_obj_set_size(lang_btn, 86, 37);
    lv_obj_align(lang_btn, LV_ALIGN_BOTTOM_RIGHT, -9, -10);
    lv_obj_add_event_cb(lang_btn, lang_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ll = lv_label_create(lang_btn);
    lv_obj_center(ll);
    lv_timer_create(ui_timer, 1000, NULL);
    refresh();
    lvgl_port_unlock();
}
