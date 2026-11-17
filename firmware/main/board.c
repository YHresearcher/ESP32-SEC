#include "board.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_touch_xpt2046.h"
#include "esp_lvgl_port.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

static const char *TAG = "SEC_BOARD";
esp_err_t sec_board_init(lv_display_t **out_display) {
    if (!out_display) return ESP_ERR_INVALID_ARG;
    *out_display = NULL;
    const spi_bus_config_t bus = {
        .sclk_io_num = SEC_SPI_SCLK,
        .mosi_io_num = SEC_SPI_MOSI,
        .miso_io_num = SEC_SPI_MISO,
        .quadwp_io_num = -1, .quadhd_io_num = -1,
        .max_transfer_sz = SEC_LCD_WIDTH * 40 * sizeof(uint16_t),
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO), TAG, "SPI init");
    esp_lcd_panel_io_handle_t lcd_io = NULL;
    const esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num = SEC_LCD_CS, .dc_gpio_num = SEC_LCD_DC,
        .spi_mode = 0, .pclk_hz = 20000000, .trans_queue_depth = 10,
        .lcd_cmd_bits = 8, .lcd_param_bits = 8,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_cfg, &lcd_io), TAG, "LCD IO");
    esp_lcd_panel_handle_t panel = NULL;
    const esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = SEC_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7789(lcd_io, &panel_cfg, &panel), TAG, "panel");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(panel), TAG, "panel reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(panel), TAG, "panel init");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(panel, false), TAG, "normal color");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_swap_xy(panel, false), TAG, "swap");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(panel, false, false), TAG, "mirror");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(panel, true), TAG, "display on");
    const lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_RETURN_ON_ERROR(lvgl_port_init(&port_cfg), TAG, "LVGL");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = lcd_io,
        .panel_handle = panel,
        .buffer_size = SEC_LCD_WIDTH * 40,
        .double_buffer = true,
        .hres = SEC_LCD_WIDTH, .vres = SEC_LCD_HEIGHT,
        .monochrome = false,
        .rotation = { .swap_xy = false, .mirror_x = false, .mirror_y = false },
        .flags = { .buff_dma = true, .buff_spiram = false },
    };
    lv_display_t *display = lvgl_port_add_disp(&disp_cfg);
    if (!display) return ESP_FAIL;
    *out_display = display;

    const gpio_config_t irq_cfg = {
        .pin_bit_mask = (1ULL << SEC_TOUCH_IRQ),
        .mode = GPIO_MODE_INPUT, .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&irq_cfg), TAG, "touch IRQ");
    esp_lcd_panel_io_handle_t touch_io = NULL;
    const esp_lcd_panel_io_spi_config_t touch_io_cfg = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(SEC_TOUCH_CS);
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi(SPI2_HOST, &touch_io_cfg, &touch_io), TAG, "touch IO");
    const esp_lcd_touch_config_t touch_cfg = {
        .x_max = SEC_LCD_WIDTH, .y_max = SEC_LCD_HEIGHT,
        .rst_gpio_num = GPIO_NUM_NC, .int_gpio_num = GPIO_NUM_NC,
        .flags = { .swap_xy = 0, .mirror_x = 1, .mirror_y = 0 },
    };
    esp_lcd_touch_handle_t touch = NULL;
    ESP_RETURN_ON_ERROR(esp_lcd_touch_new_spi_xpt2046(touch_io, &touch_cfg, &touch), TAG, "touch");
    const lvgl_port_touch_cfg_t lv_touch_cfg = { .disp = display, .handle = touch };
    if (!lvgl_port_add_touch(&lv_touch_cfg)) return ESP_FAIL;
    ESP_LOGI(TAG, "Chibi ST7789V/XPT2046 pin profile loaded; BL GPIO not driven");
    return ESP_OK;
}
