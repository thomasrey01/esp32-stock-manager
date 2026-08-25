#include "ticker_storage.h"

#define NVS_NAMESPACE "stocks"
#define NVS_KEY "tickers"

#define MAX_BUF 512

static char tickers[MAX_BUF];

void ticker_storage_init(void)
{
    memset(tickers, 0, 512);
}

const char *tickers_storage_get(void)
{
    return tickers;
}

esp_err_t ticker_storage_set(const char *json)
{
    if (sizeof(json) > MAX_BUF - 1) {
        return 1;
    }
    strncpy(tickers, json, sizeof(json) - 1);
    tickers[sizeof(json)] = '\0';

    return 0;
}

esp_err_t ticker_storage_load()
{
    nvs_handle_t handle;

    nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);

    size_t len = sizeof(tickers);

    esp_err_t err = nvs_get_str(
        handle,
        NVS_KEY,
        tickers,
        &len
    );


    nvs_close(handle);

    return err;
}

esp_err_t ticker_storage_save()
{
    nvs_handle_t handle;

    ESP_ERROR_CHECK(
        nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle)
    );

    esp_err_t err = nvs_set_str(handle, NVS_KEY, tickers);

    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }

    nvs_close(handle);

    return err;
}