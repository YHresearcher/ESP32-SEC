#include "ble_scan.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nimble_port.h"
#include "nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/util/util.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "SEC_BLE";
static volatile bool ready = false;
static sec_ble_device_t *scan_out;
static uint16_t scan_count;
static void on_sync(void) {
    int rc = ble_hs_util_ensure_addr(0);
    ready = (rc == 0);
    ESP_LOGI(TAG, "NimBLE sync: %d", rc);
}
static void ble_host_task(void *arg) {
    (void)arg;
    nimble_port_run();
    nimble_port_freertos_deinit();
}
esp_err_t sec_ble_init(void) {
    esp_err_t rc = nimble_port_init();
    if (rc != ESP_OK) return rc;
    ble_hs_cfg.sync_cb = on_sync;
    nimble_port_freertos_init(ble_host_task);
    return ESP_OK;
}
static int ble_event(struct ble_gap_event *ev, void *arg) {
    (void)arg;
    if (ev->type != BLE_GAP_EVENT_DISC || scan_out == NULL) return 0;
    const struct ble_gap_disc_desc *d = &ev->disc;
    char addr[18];
    snprintf(addr, sizeof(addr), "%02X:%02X:%02X:%02X:%02X:%02X",
        d->addr.val[5],d->addr.val[4],d->addr.val[3],
        d->addr.val[2],d->addr.val[1],d->addr.val[0]);
    for(uint16_t i=0;i<scan_count;i++) {
        if(strcmp(scan_out[i].address,addr)==0) {
            scan_out[i].rssi = d->rssi; return 0;
        }
    }
    if(scan_count >= SEC_MAX_BLE) return 0;
    sec_ble_device_t *item = &scan_out[scan_count++];
    snprintf(item->address,sizeof(item->address),"%s",addr);
    item->rssi = d->rssi;
    struct ble_hs_adv_fields f;
    if(ble_hs_adv_parse_fields(&f,d->data,d->length_data)==0 && f.name_len) {
        size_t n = f.name_len < sizeof(item->name)-1 ? f.name_len : sizeof(item->name)-1;
        memcpy(item->name,f.name,n);
        item->name[n] = 0;
    } else snprintf(item->name,sizeof(item->name),"<unnamed>");
    return 0;
}
esp_err_t sec_ble_scan(sec_ble_device_t *out, uint16_t *count) {
    if (!out || !count) return ESP_ERR_INVALID_ARG;
    *count = 0;
    if (!ready) return ESP_ERR_INVALID_STATE;
    uint8_t own_addr_type;
    if (ble_hs_id_infer_auto(0,&own_addr_type) != 0) return ESP_FAIL;
    scan_out=out;
    scan_count=0;
    memset(out,0,sizeof(sec_ble_device_t)*SEC_MAX_BLE);
    struct ble_gap_disc_params p = {0};
    p.passive = 1; p.filter_duplicates = 1;
    p.itvl = 0x50; p.window = 0x30;
    int rc = ble_gap_disc(own_addr_type, 5000, &p, ble_event, NULL);
    if(rc != 0) {scan_out=NULL;return ESP_FAIL;}
    vTaskDelay(pdMS_TO_TICKS(5250));
    ble_gap_disc_cancel();
    *count=scan_count;
    scan_out=NULL;
    return ESP_OK;
}
