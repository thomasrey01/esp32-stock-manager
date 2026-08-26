#ifndef TICKER_STORAGE
#define TICKER_STORAGE

#include "nvs.h"
#include "nvs_flash.h"
#include "string.h"

#define NUM_STORAGE 2 

#define NVS_TICKERS "tickers"
#define NVS_PRICE "price"

#define TICKER_MAX_BUF 512
#define PRICE_MAX_BUF 1024


typedef enum {
    TICKERS,
    PRICES,
} storage_type_t;

esp_err_t ticker_storage_save(storage_type_t type);
esp_err_t ticker_storage_load(size_t *len, storage_type_t type);
esp_err_t ticker_storage_set(const char *json, storage_type_t type);
const char *ticker_storage_get(storage_type_t type);
void ticker_storage_init(void);

#endif