#ifndef TICKER_STORAGE
#define TICKER_STORAGE

#include "nvs.h"
#include "nvs_flash.h"
#include "string.h"

esp_err_t ticker_storage_save();
esp_err_t ticker_storage_load();
esp_err_t ticker_storage_set(const char *json);
const char *tickers_storage_get(void);
void ticker_storage_init(void);

#endif