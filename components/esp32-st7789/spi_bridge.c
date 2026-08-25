#include "spi_bridge.h"


static spi_device_handle_t spi;

static const char *TAG = "spi_bridge";

esp_err_t ret;

uint8_t spi_init(void)
{
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_NUM_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 240 * 320 * 2,
    };

    ret = spi_bus_initialize(
        SPI2_HOST,
        &buscfg,
        SPI_DMA_CH_AUTO
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %d", ret);
        return 1;
    }

    ESP_ERROR_CHECK(ret);

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10* 1000 * 1000,
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1,
    };


    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device failed: %d", ret);
        return 1;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_NUM_DC)    |
                        (1ULL << PIN_NUM_BLK)   |
                        (1ULL << PIN_NUM_RST),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf));

    return 0;
}

uint8_t spi_denit(void)
{

    ret = spi_bus_remove_device(spi);

    return ret == ESP_OK ? 0 : 1;
}

uint8_t spi_write_cmd(uint8_t *buf, uint16_t len)
{
    if (len == 0) return 0;
    spi_transaction_t trans;
    memset(&trans, 0, sizeof(trans));

    trans.length = (size_t)len * 8;
    trans.tx_buffer = buf; 
    
    // Try blocking transaction for now

    ret = spi_device_transmit(spi, &trans);

    return ret == ESP_OK ? 0 : 1;
    
}

uint8_t cmd_data_gpio_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << PIN_NUM_DC,
        .mode = GPIO_MODE_OUTPUT,
    };
    esp_err_t ret = gpio_config(&io_conf);
    return ret == ESP_OK ? 0 : 1;
}

uint8_t cmd_data_gpio_deinit(void)
{
    esp_err_t ret = gpio_reset_pin(PIN_NUM_DC);
    return ret == ESP_OK ? 0 : 1;
}

uint8_t cmd_data_gpio_write(uint8_t value)
{
    esp_err_t ret = gpio_set_level(PIN_NUM_DC, value);
    return ret == ESP_OK ? 0 : 1;
}

uint8_t reset_gpio_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << PIN_NUM_RST,
        .mode = GPIO_MODE_OUTPUT,
    };
    esp_err_t ret = gpio_config(&io_conf);
    return ret == ESP_OK ? 0 : 1;
}

uint8_t reset_gpio_deinit(void)
{
    esp_err_t ret = gpio_reset_pin(PIN_NUM_RST);
    return ret == ESP_OK ? 0 : 1;
}

uint8_t reset_gpio_write(uint8_t value)
{
    esp_err_t ret = gpio_set_level(PIN_NUM_RST, value);
    return ret == ESP_OK ? 0 : 1;
}

void delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void debug_print(const char *const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}