#include "ui.h"
#include "wifi_scan.h"
#include "ble_scan.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"
#include "nvs.h"
#include "esp_heap_caps.h"
#include "esp_psram.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef enum { LANG_VI, LANG_EN } lang_t;
typedef enum { PAGE_HOME, PAGE_WIFI, PAGE_BLE, PAGE_DEFENSE,
               PAGE_ANALYTICS, PAGE_DIAG, PAGE_SETTINGS } page_t;
static lang_t lang = LANG_VI;
static page_t page = PAGE_HOME;
static lv_obj_t *root, *heading, *panel, *body, *back_btn, *start_btn, *lang_btn, *tiles[6];
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
static const char *menus[2][6] = {
    // ASCII fallbacks until verified Vietnamese glyph font has been generated.
    {"Wi-Fi", "Bluetooth", "Bao ve", "Phan tich", "Chan doan", "Cai dat"},
    {"Wi-Fi", "Bluetooth", "Defense", "Analytics", "Diagnostics", "Settings"},
};
static const char *page_names[2][7] = {
    {"ESP32-SEC", "Wi-Fi", "Bluetooth", "Bao ve", "Phan tich", "Chan doan", "Cai dat"},
    {"ESP32-SEC", "Wi-Fi", "Bluetooth", "Defense", "Analytics", "Diagnostics", "Settings"},
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
        lv_label_set_text(body, lang == LANG_VI ? "Quet Wi-Fi truoc de co du lieu." : "Run a Wi-Fi scan first.");
        return;
    }
    int channel_counts[15]={0}, strongest[15]={0};
    for (uint16_t i=0;i<ap_count;i++) {
        unsigned ch=results[i].channel;
        if(ch<1 || ch>14) continue;
        channel_counts[ch]++;
        if(strongest[ch]==0 || results[i].rssi>strongest[ch]) strongest[ch]=results[i].rssi;
    }
    size_t used=(size_t)snprintf(report,sizeof(report),"2.4 GHz channels\\n%u APs\\n\\n",ap_count);
    for(int ch=1;ch<=14 && used<sizeof(report)-70;ch++){
        int n=snprintf(report+used,sizeof(report)-used,
            "CH%02d  %2d APs  %4d dBm\\n",ch,channel_counts[ch],strongest[ch]);
        if(n<=0 || (size_t)n>=sizeof(report)-used)break;
        used+=(size_t)n;
    }
    lv_label_set_text(body,report);
}
static void render_defense(void) {
    if (!scan_ready || scan_result!=ESP_OK) {
        lv_label_set_text(body,lang == LANG_VI ? "Quet Wi-Fi truoc de danh gia." : "Run Wi-Fi scan before assessment.");
        return;
    }
    unsigned open=0, repeated=0;
    for(uint16_t i=0;i<ap_count;i++){
        if(results[i].authmode==0)open++;
        for(uint16_t j=0;j<i;j++){
            if(results[i].ssid[0] && strcmp(results[i].ssid,results[j].ssid)==0 &&
               strcmp(results[i].bssid,results[j].bssid)!=0) {repeated++;break;}
        }
    }
    snprintf(report,sizeof(report),
        "%u access points\\n%u open networks observed\\n%u repeated SSIDs\\n\\n"
        "Notes:\\nOpen network = no Wi-Fi encryption.\\n"
        "Repeated SSID is common in mesh\\nand enterprise networks; it\\nis NOT proof of an attack.\\n"
        "Assess only authorized networks.",ap_count,open,repeated);
    lv_label_set_text(body,report);
}
static void refresh(void);
static void back_cb(lv_event_t *e) { (void)e; page = PAGE_HOME; refresh(); }
static void lang_cb(lv_event_t *e) {
    (void)e; lang = lang == LANG_VI ? LANG_EN : LANG_VI;
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
    if (ble_busy) { lv_label_set_text(body, "Scanning BLE advertisements..."); return; }
    if (!ble_ready) { lv_label_set_text(body, "Tap Scan to discover BLE devices."); return; }
    if (ble_result != ESP_OK) {
        lv_label_set_text(body, "BLE scan error / not ready."); return;
    }
    size_t used = (size_t)snprintf(report,sizeof(report), "%u BLE devices\\n\\n",ble_count);
    for (uint16_t i=0; i<ble_count && i<12 && used < sizeof(report)-90; i++) {
        int n = snprintf(report+used,sizeof(report)-used,"%s\\n%d dBm  %s\\n",
            ble_results[i].name, ble_results[i].rssi, ble_results[i].address);
        if(n<=0 || (size_t)n>=sizeof(report)-used) break;
        used+=(size_t)n;
    }
    lv_label_set_text(body,report);
}
static void start_cb(lv_event_t *e) {
    (void)e;
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
    } else lv_label_set_text(body, lang == LANG_VI ? "Dang quet..." : "Scanning...");
}
static void render_wifi(void) {
    if (scan_busy) {
        lv_label_set_text(body, lang == LANG_VI ? "Dang quet Wi-Fi..." : "Scanning Wi-Fi...");
        return;
    }
    if (!scan_ready) {
        lv_label_set_text(body, lang == LANG_VI ? "Nhan Scan de tim mang." : "Tap Scan to discover APs.");
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
    for (int i=0;i<6;i++) {
        if (home) lv_obj_remove_flag(tiles[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(tiles[i], LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(lv_obj_get_child(tiles[i],0), menus[lang][i]);
    }
    if (home) {
        lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(back_btn, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(back_btn, LV_OBJ_FLAG_HIDDEN);
    }
    if (page == PAGE_WIFI || page == PAGE_BLE) lv_obj_remove_flag(start_btn, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(start_btn, LV_OBJ_FLAG_HIDDEN);
    if (page == PAGE_SETTINGS) lv_obj_remove_flag(lang_btn, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(lang_btn, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(lv_obj_get_child(lang_btn,0), lang == LANG_VI ? "EN" : "VI");
    lv_label_set_text(lv_obj_get_child(start_btn,0), ((page==PAGE_WIFI && scan_busy) || (page==PAGE_BLE && ble_busy)) ? "Stop" : "Scan");
    if (page == PAGE_WIFI) render_wifi();
    if (page == PAGE_BLE) lv_label_set_text(body, lang == LANG_VI ?
        "BLE Explorer: chua tich hop.\nChua co quet BLE." :
        "BLE Explorer not yet implemented.\nNo BLE scan available.");
    if (page == PAGE_DEFENSE) lv_label_set_text(body,
        lang == LANG_VI ? "Defense: cho module\ngiam sat thu dong." :
                          "Defense: passive monitoring\nmodule pending.");
    if (page == PAGE_ANALYTICS) lv_label_set_text(body,
        lang == LANG_VI ? "Phan tich kenh: dang phat trien." :
                          "Channel analytics: pending.");
    if (page == PAGE_DIAG) {
        snprintf(diag, sizeof(diag),
            "ESP32-S3 N16R8\nPSRAM: %u KB\nFree heap: %u KB\nLCD: ST7789V 240x320\nTouch: XPT2046\nBL: not controlled\nFW: M1-M3 dev",
            (unsigned)(esp_psram_get_size()/1024), (unsigned)(esp_get_free_heap_size()/1024));
        lv_label_set_text(body, diag);
    }
    if (page == PAGE_SETTINGS) lv_label_set_text(body, lang == LANG_VI ?
        "Ngon ngu: VI\nDoi sang English bang nut EN.\n\nChua co font Viet co dau." :
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
    heading = lv_label_create(root);
    lv_obj_set_style_text_font(heading, &lv_font_montserrat_18, 0);
    lv_obj_align(heading, LV_ALIGN_TOP_MID, 0, 9);
    for (int i=0;i<6;i++) {
        tiles[i] = lv_button_create(root);
        lv_obj_set_size(tiles[i], 106, 67);
        lv_obj_set_pos(tiles[i], 10+(i%2)*114, 42+(i/2)*80);
        lv_obj_set_style_bg_color(tiles[i], lv_color_hex(0x173A55), 0);
        lv_obj_add_event_cb(tiles[i], tile_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
        lv_obj_t *label = lv_label_create(tiles[i]);
        lv_obj_center(label);
    }
    body = lv_label_create(root);
    lv_obj_set_width(body, 218);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_obj_align(body, LV_ALIGN_TOP_MID, 0, 47);
    back_btn = lv_button_create(root);
    lv_obj_set_size(back_btn, 86, 37);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 9, -10);
    lv_obj_add_event_cb(back_btn, back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(back_btn);
    lv_label_set_text(bl, "Back"); lv_obj_center(bl);
    start_btn = lv_button_create(root);
    lv_obj_set_size(start_btn, 86, 37);
    lv_obj_align(start_btn, LV_ALIGN_BOTTOM_RIGHT, -9, -10);
    lv_obj_add_event_cb(start_btn, start_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *sl = lv_label_create(start_btn);
    lv_label_set_text(sl, "Scan"); lv_obj_center(sl);
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
