#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "soc/soc_caps.h"
#include "esp_freertos_hooks.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_rgb.h"
#include "driver/gpio.h"
#include "board.h"
#include "lvgl.h"
#include "esp_timer.h"

#define TAG "RGB"

static esp_lcd_panel_handle_t g_panel_handle = NULL;

static void __qsmd_rgb_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;

    esp_lcd_panel_draw_bitmap(g_panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_p);
    lv_disp_flush_ready(disp_drv);
}

void qmsd_rgb_init(esp_lcd_rgb_panel_config_t *panel_config)
{
    static lv_disp_drv_t disp_drv;
    int buffer_size;
    void *buf1 = NULL;
    void *buf2 = NULL;
	static lv_disp_draw_buf_t draw_buf;

    lv_init();

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(panel_config, &g_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(g_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(g_panel_handle));

    buffer_size = 480 * 40;
	buf1 = heap_caps_malloc(buffer_size * 2, MALLOC_CAP_DMA);
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buffer_size);

    lv_disp_drv_init(&disp_drv);         
    disp_drv.flush_cb = __qsmd_rgb_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.hor_res = panel_config->timings.h_res;
    disp_drv.ver_res = panel_config->timings.v_res;
    lv_disp_drv_register(&disp_drv);
}

void screen_init(void) {
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << LCD_PIN_BK_LIGHT
    };
    // Initialize the GPIO of backlight
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    ESP_ERROR_CHECK(gpio_set_level(LCD_PIN_BK_LIGHT, LCD_BK_LIGHT_OFF_LEVEL));

    esp_lcd_rgb_panel_config_t panel_config = {
        .data_width = 16,
        .psram_trans_align = 64,
        .pclk_gpio_num = LCD_PCLK_GPIO,
        .vsync_gpio_num = LCD_VSYNC_GPIO,
        .hsync_gpio_num = LCD_HSYNC_GPIO,
        .de_gpio_num = LCD_DE_GPIO,
        .disp_gpio_num = LCD_DISP_EN_GPIO,
        .data_gpio_nums = {
            LCD_DATA0_GPIO,
            LCD_DATA1_GPIO,
            LCD_DATA2_GPIO,
            LCD_DATA3_GPIO,
            LCD_DATA4_GPIO,
            LCD_DATA5_GPIO,
            LCD_DATA6_GPIO,
            LCD_DATA7_GPIO,
            LCD_DATA8_GPIO,
            LCD_DATA9_GPIO,
            LCD_DATA10_GPIO,
            LCD_DATA11_GPIO,
            LCD_DATA12_GPIO,
            LCD_DATA13_GPIO,
            LCD_DATA14_GPIO,
            LCD_DATA15_GPIO,
        },
        .timings = {
            .pclk_hz = 14000000,
            .h_res = 480,
            .v_res = 272,
            .hsync_pulse_width = 10,
            .hsync_back_porch = 80,
            .hsync_front_porch = 60,    // 630
            .vsync_pulse_width = 10,
            .vsync_back_porch = 20,
            .vsync_front_porch = 60,    // 362
            .flags.pclk_active_neg = 1,
        },
        .flags.fb_in_psram = 1,
        .flags.double_fb = 0,
        .flags.refresh_on_demand = 0,   // Mannually control refresh operation
        .bounce_buffer_size_px = 0,
        .clk_src = LCD_CLK_SRC_PLL160M,
    };

    qmsd_rgb_init(&panel_config);

    ESP_ERROR_CHECK(gpio_set_level(LCD_PIN_BK_LIGHT, LCD_BK_LIGHT_ON_LEVEL));
}
