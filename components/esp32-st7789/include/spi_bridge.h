#ifndef SPI_BRIDGE
#define SPI_BRIDGE

#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "esp_log.h"


#define PIN_NUM_MOSI    GPIO_NUM_21
#define PIN_NUM_SCLK    GPIO_NUM_18
#define PIN_NUM_CS      GPIO_NUM_22
#define PIN_NUM_DC      GPIO_NUM_19
#define PIN_NUM_RST     GPIO_NUM_2
#define PIN_NUM_BLK     GPIO_NUM_23
#define PIN_NUM_RES     GPIO_NUM_2



uint8_t spi_init(void);
uint8_t spi_denit(void);
uint8_t spi_write_cmd(uint8_t *buf, uint16_t len);
uint8_t cmd_data_gpio_init(void);
uint8_t cmd_data_gpio_deinit(void);
uint8_t cmd_data_gpio_write(uint8_t value);
uint8_t reset_gpio_init(void);
uint8_t reset_gpio_deinit(void);
uint8_t reset_gpio_write(uint8_t value);
void delay_ms(uint32_t ms);
void debug_print(const char *const fmt, ...);

#endif