#pragma once
#include "esp_err.h"
#include "lvgl.h"
// GPIOs taken from working ModESP32 Chibi ST7789V/XPT2046 profile.
#define SEC_LCD_WIDTH 240
#define SEC_LCD_HEIGHT 320
#define SEC_SPI_SCLK 12
#define SEC_SPI_MOSI 11
#define SEC_SPI_MISO 8
#define SEC_LCD_CS 10
#define SEC_LCD_DC 13
#define SEC_LCD_RST 14
#define SEC_TOUCH_CS 9
#define SEC_TOUCH_IRQ 7
// Backlight GPIO intentionally unset: existing Chibi config leaves it unverified.
esp_err_t sec_board_init(lv_display_t **out_display);
