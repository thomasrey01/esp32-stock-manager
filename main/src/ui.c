#include "ui.h"

#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"

#include "driver_st7789.h"
#include "driver_st7789_font.h"
#include "spi_bridge.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "day_time_handler.h"

#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  320

#define LVGL_BUFFER_LINES  40

#define NUM_SCREEN_OBJECTS 15

#define NUM_STOCKS 3 // for now the display only supports 3 different stocks

st7789_handle_t st_handle;

static const char *TAG = "UI";

static lv_display_t *display;

static uint16_t lvgl_buf[DISPLAY_WIDTH * LVGL_BUFFER_LINES];

QueueSetHandle_t ui_queue;

typedef enum {
    OBJECT_STOCK_TICKER1,
    OBJECT_STOCK_TICKER2,
    OBJECT_STOCK_TICKER3,
    OBJECT_STOCK_PRICE1,
    OBJECT_STOCK_PRICE2,
    OBJECT_STOCK_PRICE3,
    OBJECT_STOCK_PRICE_CHANGE1,
    OBJECT_STOCK_PRICE_CHANGE2,
    OBJECT_STOCK_PRICE_CHANGE3,
    OBJECT_IP_ADDRESS,
    OBJECT_CLOCK,
    OBJECT_CPU_STATS1,
    OBJECT_CPU_STATS2,
    OBJECT_CPU_STATS3,
    OBJECT_WIFI_STATUS,
} ui_obj_id_t;

lv_obj_t* screen_objects[NUM_SCREEN_OBJECTS];

lv_obj_t* app_name;
lv_obj_t* preload;
lv_obj_t* wifi_status;

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
    // st7789_memory_write(&st_handle, (uint8_t *)pixels, pixel_count * 2);
    st7789_memory_write(&st_handle, px_map, pixel_count * 2);

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

    st7789_display_inversion_on(&st_handle);

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

    app_name = lv_label_create(lv_screen_active());

    lv_label_set_text(app_name, "ESP Stock \nManagement v1.0.0");
    lv_obj_center(app_name);
    lv_obj_set_style_text_color(app_name, lv_color_hex(0xFFFFFDF), LV_PART_MAIN);

    preload = lv_spinner_create(lv_screen_active());

    lv_obj_set_size(preload, 40, 40);
    lv_obj_align(preload, LV_ALIGN_BOTTOM_LEFT, 40, 0);

    wifi_status = lv_label_create(lv_screen_active());

    lv_label_set_text(wifi_status, "Connecting to \nwifi...");
    lv_obj_align(wifi_status, LV_ALIGN_BOTTOM_LEFT, 85, 0);
    lv_obj_set_style_text_color(wifi_status, lv_color_hex(0xFFFFFDF), LV_PART_MAIN);


    lvgl_port_unlock();

}

void ui_wifi_ready(const char *address)
{   
    static bool first_update = true;

    ESP_LOGI(TAG, "Updating ip address to: %s\n", address);

    lv_obj_t *ip_address = screen_objects[OBJECT_IP_ADDRESS];

    lvgl_port_lock(0);

    if (first_update) {

        lv_obj_delete(app_name);
        lv_obj_delete(preload);
        lv_obj_delete(wifi_status);



        lv_obj_set_style_text_color(ip_address, lv_color_hex(0xFFFFFF), 0);

        lv_obj_remove_flag(screen_objects[OBJECT_IP_ADDRESS], LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(screen_objects[OBJECT_CLOCK], LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(screen_objects[OBJECT_CPU_STATS1], LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(screen_objects[OBJECT_CPU_STATS2], LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(screen_objects[OBJECT_CPU_STATS3], LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(screen_objects[OBJECT_WIFI_STATUS], LV_OBJ_FLAG_HIDDEN);
        
        first_update = false;
    }

    lv_label_set_text(ip_address, address);


    lvgl_port_unlock();
}

void ui_update_market(market_data_t market_data, int stock_idx)
{
    char price_str[10];
    char change_str[10];

    lv_obj_t *ticker_label = screen_objects[stock_idx];
    lv_obj_t *price_label = screen_objects[stock_idx+NUM_STOCKS];
    lv_obj_t *change_label = screen_objects[stock_idx+NUM_STOCKS*2];
    lv_color_t color;

    ESP_LOGI(
        TAG,
        "Updating UI: %s %.2f %.2f",
        market_data.ticker,
        market_data.price,
        market_data.change
    );

    if (market_data.change > 0) {
        color = lv_color_hex(0x39bd39);
    } else {
        market_data.change *= -1;
        color = lv_color_hex(0xe36b6b);
    }

    snprintf(price_str, sizeof(price_str), "$%.2f", market_data.price);
    snprintf(change_str, sizeof(change_str), "%.2f", market_data.change);


    lvgl_port_lock(0);

    lv_label_set_text(
        price_label,
        price_str
    );

    lv_label_set_text(
        change_label,
        change_str
    );

    lv_obj_set_style_text_color(
        change_label,
        color,
        LV_PART_MAIN
    );

    lv_label_set_text_fmt(
        ticker_label,
        "%s: ",
        market_data.ticker
    );

    lv_obj_remove_flag(ticker_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(price_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(change_label, LV_OBJ_FLAG_HIDDEN);

    lvgl_port_unlock();
    
}

void ui_update_cpu(cpu_stats_t cpu_stats)
{
    char* cpu_labels[] = {"cpu0", "cpu1", "avg"};
    lv_obj_t* cpu_obj;
    uint8_t red = 0;
    uint8_t green = 0;
    uint32_t color;
    
    lvgl_port_lock(0);


    for (uint8_t i = 0; i < 3; i++) {
        cpu_obj = screen_objects[OBJECT_CPU_STATS1+i];
        uint8_t cpu_val = *((uint8_t *)&cpu_stats + i);

        if (cpu_val <= 50) {
            green = UINT8_MAX;
            red = cpu_val * (UINT8_MAX / 50);
        } else {
            red = UINT8_MAX;
            green = (100 - cpu_val) * (UINT8_MAX / 50);
        }

        color = ((uint32_t)red << 16) + ((uint16_t)green << 8);

        lv_label_set_text_fmt(
            cpu_obj,
            "%s: %u%%",
            cpu_labels[i],
            cpu_val
        );

        lv_obj_set_style_text_color(
            cpu_obj,
            lv_color_hex(color),
            LV_PART_MAIN
        );

    }

    lvgl_port_unlock();


}

void ui_update_clock(clock_data_t clock_data)
{
    lv_obj_t *clock_obj = screen_objects[OBJECT_CLOCK];
    lv_color_t color;

    if (is_market_open(clock_data)) {
        color = lv_color_hex(0x80fff9);
    } else {
        color = lv_color_hex(0xffd500);
    }

    lvgl_port_lock(0);

    lv_label_set_text_fmt(
        clock_obj,
        "%.2d:%.2d:%.2d",
        clock_data.hour,
        clock_data.minute,
        clock_data.second
    );

    lv_obj_set_style_text_color(
        clock_obj,
        color,
        LV_PART_MAIN
    );

    lvgl_port_unlock();

}

void ui_update_wifi_status(int8_t wifi_data)
{
    int color;
    if (wifi_data == 1) {
        color = 0xbababa;
    } else if (wifi_data > -50) {
        color = 0xFFFFFF;
    } else if (wifi_data > -60) {
        color = 0xfffd94;
    } else if (wifi_data > -75) {
        color = 0xffbe00;
    } else {
        color = 0xff3200;
    }

    lvgl_port_lock(0);

    lv_obj_set_style_text_color(
        screen_objects[OBJECT_WIFI_STATUS],
        lv_color_hex(color),
        LV_PART_MAIN
    );

    lvgl_port_unlock();


}

static void ui_create_objects(void)
{
    lvgl_port_lock(0);
    lv_obj_t *screen = lv_screen_active();

    lv_obj_set_style_bg_color(
        screen,
        lv_color_hex(0x000000),
        0
    );

    int posy = 50;
    int posx_ticker = 40;
    int posx_price = 45;
    int posx_price_change = 135;

    for (int i = 0; i < NUM_SCREEN_OBJECTS; i++) {
        screen_objects[i] = lv_label_create(screen);
        lv_obj_add_flag(screen_objects[i], LV_OBJ_FLAG_HIDDEN);

    }
    
    for (int i = 0; i < NUM_STOCKS; i++) {

        lv_obj_set_pos(screen_objects[i], posx_ticker, posy);
        lv_obj_set_pos(screen_objects[i+NUM_STOCKS], posx_price, posy+30);
        // lv_obj_set_pos(screen_objects[i+NUM_STOCKS*2], posx_price_change, posy+33);

        lv_obj_align(screen_objects[i+NUM_STOCKS*2], LV_ALIGN_TOP_RIGHT, -50, posy+33);

        posy += 60;

        lv_obj_set_style_text_font(
            screen_objects[i],
            &lv_font_montserrat_24,
            LV_PART_MAIN
        );

        lv_obj_set_style_text_color(
            screen_objects[i],
            lv_color_hex(0x07E0),
            LV_PART_MAIN
        );

        lv_obj_set_style_text_font(
            screen_objects[i+NUM_STOCKS],
            &lv_font_montserrat_24,
            LV_PART_MAIN
        );

        lv_obj_set_style_text_color(
            screen_objects[i+NUM_STOCKS],
            lv_color_hex(0xFFFFFF),
            LV_PART_MAIN
        );

        lv_obj_set_style_text_font(
            screen_objects[i+NUM_STOCKS*2],
            &lv_font_montserrat_22,
            LV_PART_MAIN
        );

        lv_obj_set_style_text_color(
            screen_objects[i+NUM_STOCKS*2],
            lv_color_hex(0xFFFFFF),
            LV_PART_MAIN
        );
    }

    lv_obj_align(screen_objects[OBJECT_IP_ADDRESS], LV_ALIGN_BOTTOM_LEFT, 40, -5);

    lv_obj_align(screen_objects[OBJECT_CLOCK], LV_ALIGN_TOP_LEFT, 40, 10);

    lv_obj_set_style_text_font(
        screen_objects[OBJECT_CLOCK],
        &lv_font_montserrat_18,
        LV_PART_MAIN
    );

    lv_obj_set_style_text_color(
        screen_objects[OBJECT_CLOCK],
        lv_color_hex(0x80fff9),
        LV_PART_MAIN
    );

    for (uint8_t i = 0; i < 3; i++) {
        lv_obj_t* cpu_obj = screen_objects[OBJECT_CPU_STATS1 + i];
        lv_obj_align(cpu_obj, LV_ALIGN_BOTTOM_RIGHT, -40, -70 + i * (20));

        lv_obj_set_style_text_font(
            cpu_obj,
            &lv_font_montserrat_16,
            LV_PART_MAIN
        );

        lv_obj_set_style_text_color(
            cpu_obj,
            lv_color_hex(0xFFFFFF),
            LV_PART_MAIN
        );
    }

    lv_obj_align(screen_objects[OBJECT_WIFI_STATUS], LV_ALIGN_TOP_RIGHT, -40, 10);
    
    lv_obj_set_style_text_font(
        screen_objects[OBJECT_WIFI_STATUS],
        &lv_font_montserrat_16,
        LV_PART_MAIN
    );

    lv_label_set_text(screen_objects[OBJECT_WIFI_STATUS], LV_SYMBOL_WIFI);

        
    // lv_label_set_text(screen_objects[OBJECT_WIFI_STATUS], "WiFi");
    lvgl_port_unlock();
    
}


static void ui_process_message(void)
{
    ui_message_t ui_message;
    int stock_idx = 0; 

    while (xQueueReceive(ui_queue, &ui_message, 0) == pdTRUE) {

        switch (ui_message.message_type) {
            case UI_MSG_MARKET:
                ui_update_market(ui_message.market_data, stock_idx);
                stock_idx++;
                break;
            
            case UI_MSG_CLOCK:
                ui_update_clock(ui_message.clock_data);
                break;

            case UI_MSG_CPU:
                ui_update_cpu(ui_message.cpu_stats);
                break;

            case UI_MSG_WIFI_STATUS:
                ui_update_wifi_status(ui_message.wifi_data);
                break;

            case UI_MSG_WIFI_CONNECT:
                ui_wifi_ready(ui_message.ip_addr);
                break;

            default:
                break;
        }

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

    ui_create_objects();

    return ESP_OK;
}