#include "ticker_storage.h"
#include "esp_log.h"

#define NVS_NAMESPACE "stocks"


static char tickers[TICKER_MAX_BUF];
static char prices[PRICE_MAX_BUF];

static char* storage[NUM_STORAGE];

static char *keys[NUM_STORAGE];

static bool is_init = false;

void ticker_storage_init(void)
{
    if (is_init) {
        return;
    }

    keys[TICKERS] = NVS_TICKERS;
    keys[PRICES] = NVS_PRICE;

    storage[TICKERS] = tickers;
    storage[PRICES] = prices;
    memset(tickers, 0, TICKER_MAX_BUF);
    memset(prices, 0, PRICE_MAX_BUF);
    is_init = true;
}


const char *ticker_storage_get(storage_type_t type)
{
    return storage[type];
}

esp_err_t ticker_storage_set(const char *json, storage_type_t type)
{

    size_t len = strlen(json);
    if (len+1 > TICKER_MAX_BUF - 1) {
        return 1;
    }

    ESP_LOGI("TICKER", "setting string to %s with size: %d", json, len);
    strncpy(storage[type], json, len);
    tickers[len] = '\0';

    return 0;
}

esp_err_t ticker_storage_load(size_t *len, storage_type_t type)
{
    nvs_handle_t handle;

    nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);

    esp_err_t err = nvs_get_str(
        handle,
        keys[type],
        storage[type],
        len
    );

    nvs_close(handle);

    return err;
}

esp_err_t ticker_storage_save(storage_type_t type)
{
    nvs_handle_t handle;

    ESP_LOGI("TICKER", "saving string: %s with length %d", storage[type], sizeof(tickers));

    ESP_ERROR_CHECK(
        nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle)
    );

    esp_err_t err = nvs_set_str(handle, keys[type], storage[type]);

    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }

    nvs_close(handle);

    return err;
}