#include "ui.h"

#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"

#include "driver_st7789.h"
#include "driver_st7789_font.h"
#include "spi_bridge.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  320

#define LVGL_BUFFER_LINES  40

st7789_handle_t st_handle;

static const char *TAG = "UI";

static lv_display_t *display;

static int num_tickers = 0;

static uint16_t lvgl_buf[DISPLAY_WIDTH * LVGL_BUFFER_LINES];

QueueSetHandle_t ui_queue;

static void lvgl_flush_cb(
    lv_display_t *disp,
    const lv_area_t *area,
    uint8_t *px_map
)
{
    uint32_t pixel_count = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);
    uint16_t *pixels = (uint16_t *)px_map;

    for (uint32_t i = 0; i < pixel_count; i++) {
        pixels[i] = (pixels[i] >> 8) | (pixels[i] << 8);
    }

    st7789_set_column_address(&st_handle, area->x1, area->x2);
    st7789_set_row_address(&st_handle, area->y1, area->y2);
    st7789_memory_write(&st_handle, (uint8_t *)pixels, pixel_count * 2);

    lv_display_flush_ready(disp);
}


static void init_display()
{
    st_handle = (st7789_handle_t){
        .spi_init = spi_init,
        .spi_deinit = spi_denit,
        .spi_write_cmd = spi_write_cmd,
        .cmd_data_gpio_init = cmd_data_gpio_init,
        .cmd_data_gpio_deinit = cmd_data_gpio_deinit,
        .cmd_data_gpio_write = cmd_data_gpio_write,
        .reset_gpio_init = reset_gpio_init,
        .reset_gpio_deinit = reset_gpio_deinit,
        .reset_gpio_write = reset_gpio_write,
        .debug_print = debug_print,
        .delay_ms = delay_ms,
    };

    uint8_t err = st7789_init(&st_handle);

    if (err != 0) {
        ESP_LOGE(TAG, "ST init failed with value: %d\n", err);
    } else {
        ESP_LOGI(TAG, "ST init successful\n");
    }

    st7789_set_column(&st_handle, 240);
    st7789_set_row(&st_handle, 320);

    gpio_set_level(PIN_NUM_BLK, 1);

    err = st7789_sleep_out(&st_handle);
    ESP_LOGI(TAG, "sleep out: %d", err);

    vTaskDelay(pdMS_TO_TICKS(120));


    err = st7789_set_interface_pixel_format(&st_handle, ST7789_RGB_INTERFACE_COLOR_FORMAT_65K, ST7789_CONTROL_INTERFACE_COLOR_FORMAT_16_BIT);

    ESP_LOGI(TAG, "pixel format: %d", err);

    err = st7789_set_memory_data_access_control(
    &st_handle,
    ST7789_ORDER_COLOR_RGB
    );
    ESP_LOGI(TAG, "MADCTL: %d", err);

    err = st7789_display_on(&st_handle);
    ESP_LOGI(TAG, "display on: %d", err);

    st7789_clear(&st_handle);

    err = st7789_write_string(&st_handle, 42, 32, "Hello", 5, 0x368F, ST7789_FONT_24);
    vTaskDelay(pdMS_TO_TICKS(120));
    err = st7789_write_string(&st_handle, 42, 50, "There", 6, 0x368F, ST7789_FONT_24);
    vTaskDelay(pdMS_TO_TICKS(120));

    if (err != 0) {
        ESP_LOGE(TAG, "write failed with value: %d\n", err);
    } else {
        ESP_LOGI(TAG, "write successful\n");
    }

    ESP_LOGI(TAG, "Finish display init");
    vTaskDelay(pdMS_TO_TICKS(500));
}


static void ui_create_start_screen(void)
{
    lvgl_port_lock(0);

    lv_obj_t *app_name =
        lv_label_create(lv_screen_active());

    lv_label_set_text(app_name, "ESP Stock \nManagement v1.0.0");

    lv_obj_center(app_name);

    lv_obj_t *preload = lv_spinner_create(lv_screen_active());

    lv_obj_set_size(preload, 40, 40);
    lv_obj_align(preload, LV_ALIGN_BOTTOM_LEFT, 40, 0);

    lv_obj_t * wifi_status = lv_label_create(lv_screen_active());
    lv_label_set_text(wifi_status, "Connecting to \nwifi...");
    lv_obj_align(wifi_status, LV_ALIGN_BOTTOM_LEFT, 85, 0);

    lvgl_port_unlock();

}

void ui_wifi_ready(const char *address)
{
    lvgl_port_lock(0);

    lv_obj_t *screen = lv_screen_active();
    lv_obj_clean(screen);

    lv_obj_t *wifi =
        lv_label_create(screen);

    lv_label_set_text(wifi, address);

    lv_obj_align(wifi, LV_ALIGN_BOTTOM_LEFT, 40, -5);

    lvgl_port_unlock();


}

static void ui_process_message(void)
{
    market_data_t market_data;
    char price_str[10];
    int posy = 30;

    while (xQueueReceive(ui_queue, &market_data, 0) == pdTRUE) {
        lv_obj_t *ticker_label = lv_label_create(lv_screen_active());
        lv_obj_t *price_label = lv_label_create(lv_screen_active());

        snprintf(price_str, sizeof(price_str), "$%.2f", (double)market_data.price);

        lv_label_set_text(
            price_label,
            price_str
        );

        lv_obj_set_style_text_font(
            price_label,
            &lv_font_montserrat_24,
            LV_PART_MAIN
        );

        lv_obj_set_style_text_color(
            price_label,
            lv_color_hex(0x07E0),
            LV_PART_MAIN
        );


        ESP_LOGI(
            TAG,
            "Updating UI: %s %.2f",
            market_data.ticker,
            market_data.price
        );

        lv_label_set_text_fmt(
            ticker_label,
            "%s: ",
            market_data.ticker
        );

        lv_obj_set_style_text_font(
            ticker_label,
            &lv_font_montserrat_24,
            LV_PART_MAIN
        );

        lv_obj_set_pos(ticker_label, 50, posy);
        lv_obj_set_pos(price_label, 90, posy+30);

        posy += 60;

    }
}

void lvgl_task(void *pvParameters)
{
    for(;;) {

        ui_process_message();

        uint32_t delay_ms = lv_timer_handler();

        if (delay_ms > 100) delay_ms = 100;

        if (delay_ms < 5) delay_ms = 5;

        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

esp_err_t ui_init(void)
{
    ESP_LOGI(TAG, "Initializing UI");

    init_display();

    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();

    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    display = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);

    if (display == NULL) {
        ESP_LOGE(TAG, "Failed to create display!\n");

        return ESP_FAIL;
    }


    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);

    lv_display_set_buffers(
        display,
        lvgl_buf,
        NULL,
        sizeof(lvgl_buf),
        LV_DISPLAY_RENDER_MODE_PARTIAL
    );

    lv_display_set_flush_cb(
        display,
        lvgl_flush_cb
    );

    ui_create_start_screen(); 

    return ESP_OK;
}

void set_num_tickers(int num)
{
    num_tickers = num;
}