idf_component_register(SRCS "main.c" "board.c" "ui.c" "wifi_scan.c" "ble_scan.c" "feature_registry.c" "security_audit.c" "sec_report.c" "lab_link.c" "boot_guard.c" "sec_font_vi_13.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_lcd esp_driver_spi esp_driver_gpio esp_wifi esp_event esp_netif nvs_flash esp_psram esp_timer bt
                       PRIV_REQUIRES atanisoft__esp_lcd_touch_xpt2046 esp_lvgl_port lvgl)
