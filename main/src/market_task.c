#include "market_task.h"


#define ISDEBUG_FIRST 1

typedef enum {
    STATE_FIRST_TIME_INIT,
    STATE_INIT,
    STATE_GET_TIME,
    STATE_CHECK_MARKET,
    STATE_SAVE_NVS,
    STATE_UPDATE_MARKET_DISPLAY,
    STATE_SLEEP,
} market_state_t;


extern http_response_t response;
extern QueueSetHandle_t ui_queue;
extern QueueSetHandle_t time_queue;
extern EventGroupHandle_t wifi_event_group;


static const char* TAG = "MARKET";


// Returns the json of ticker price
/*
    {"day": "DAY", "tickers": ["TICKER1", ..."], "prices": {"TICKER1": ...}}
    right now this is only getting the ticker lists...
*/
cJSON* get_ticker_json()
{
    esp_err_t err;
    cJSON *j = NULL;

    char new_tickers[] = "{\"tickers\":[\"AMZN\",\"QBTS\",\"QUBT\"]}";

    size_t len = TICKER_MAX_BUF;

    err = ticker_storage_load(&len, TICKERS);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error getting ticker. errno: %d\n", err);
    }

    const char *ticker_str = ticker_storage_get(TICKERS);

    ESP_LOGI(TAG, "Got ticker string: %s\n", new_tickers);

    j = cJSON_Parse(new_tickers);

    ESP_LOGI(TAG, "JSON string: %s\n", cJSON_Print(j));

    return j;
}

esp_err_t save_ticker_json(cJSON *j)
{
    esp_err_t err;

    err = ticker_storage_set(cJSON_Print(j), PRICES);

    ESP_ERROR_CHECK(err);

    err = ticker_storage_save(PRICES);

    return err;

}

void market_task(void *pvParameters)
{
    static market_state_t state = STATE_INIT;
    char api_path[256];
    cJSON *j = NULL;
    char *day = NULL;
    cJSON *tickers = NULL;
    cJSON *save_tickers = NULL;
    cJSON *price_changes = NULL;
    cJSON *new_prices = NULL;
    cJSON *res = NULL;
    cJSON *time_api_resp;
    cJSON *stock_api_resp;
    char day_current[10];
    char *time_current = NULL;
    char * ticker_str = NULL;
    bool successful_api;
    
    cJSON *results_item;
    cJSON *p;
    double price_value;
    float price_change;

    bool first_call = true;

    ui_message_t ui_message = {0};
    clock_data_t clock_data = {0};


    esp_err_t err;

    for (;;) {
        switch(state) {

            case STATE_INIT:

                ESP_LOGI(TAG, "In INIT\n");

                ticker_storage_init();
                
                j = get_ticker_json(); // fix this
                day = cJSON_GetStringValue(cJSON_GetObjectItem(j, "day"));
                tickers = cJSON_GetObjectItem(j, "tickers");

                state = STATE_GET_TIME;
                break;
            case STATE_GET_TIME:

                xEventGroupWaitBits(wifi_event_group, BIT0, pdFALSE, pdTRUE, pdMS_TO_TICKS(4000));

                ESP_LOGI(TAG, "In GET TIME\n");
                // First get current day
                err = https_with_hostname_path("timeapi.io", "/api/v1/time/current/zone?timeZone=America\%2FNew_York");

                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Error getting time request with errno: %d\n", err);
                    state = STATE_SLEEP;
                    break;
                }

                ESP_LOGI(TAG, "Got time api response: %s\n", response.buffer);

                time_api_resp = cJSON_ParseWithLength(response.buffer, response.length);
                ESP_LOGI(TAG, "loaded json with length %d\n", response.length);
                
                strcpy(
                    day_current,
                    cJSON_GetStringValue(cJSON_GetObjectItem(time_api_resp, "day_of_week"))
                );
                ESP_LOGI(TAG, "Current day: %s\n", day_current);

                err = parse_time(cJSON_GetStringValue(cJSON_GetObjectItem(time_api_resp, "time")), &clock_data);

                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to parse time string with errno: %d", err);
                    state = STATE_SLEEP;
                    break;
                }

                xQueueSend(time_queue, &clock_data, 0);

                ESP_LOGI(TAG, "Current time: %.2d:%.2d:%.2d\n", 
                                clock_data.hour, 
                                clock_data.minute, 
                                clock_data.second
                        );

                if (
                    first_call ||
                    (
                        get_day(day_current) != -1 &&
                        is_market_open(clock_data)
                    )
                ) {
                    first_call = false;
                    state = STATE_CHECK_MARKET;
                } else {
                    state = STATE_SLEEP;
                }



                cJSON_Delete(time_api_resp);

                break;

            case STATE_CHECK_MARKET:

                ESP_LOGI(TAG, "In check market\n");

                new_prices = cJSON_CreateObject();
                price_changes = cJSON_CreateObject();
                res = cJSON_CreateObject();

                cJSON_AddItemToObject(res, "price_changes", price_changes);
                cJSON_AddItemToObject(res, "new_prices", new_prices);

                successful_api = true;

                xEventGroupWaitBits(wifi_event_group, BIT0, pdFALSE, pdTRUE, pdMS_TO_TICKS(4000));
                
                for (int i = 0; i < cJSON_GetArraySize(tickers); i++) {

                    ticker_str = cJSON_GetStringValue(cJSON_GetArrayItem(tickers, i));

                    ESP_LOGI(TAG, "Getting ticker: %s\n", cJSON_Print(cJSON_GetArrayItem(tickers, i)));
                    
                    // snprintf(api_path, sizeof(api_path),
                    //     "/v2/aggs/ticker/%s/prev?adjusted=true&apiKey=%s",
                    //     ticker_str,
                    //     API_KEY
                    // );

                    snprintf(api_path, sizeof(api_path),
                        "/v8/finance/chart/%s?range=1d&interval=1d",
                        ticker_str
                    );

                    ESP_LOGI(TAG, "getting path: %s\n", api_path);                   
                    // err = https_with_hostname_path("api.massive.com", api_path);
                    err = https_with_hostname_path("query1.finance.yahoo.com", api_path);

                    if (err != ESP_OK) {
                        ESP_LOGE(TAG, "Error getting time request with errno: %d\n", err);
                        successful_api = false;
                    }


                    stock_api_resp = cJSON_ParseWithLength(response.buffer, response.length);

                    results_item = cJSON_GetArrayItem(cJSON_GetObjectItem(cJSON_GetObjectItem(stock_api_resp, "chart"), "result"), 0);

                    ESP_LOGI(TAG, "Result item: %s\n", cJSON_Print(results_item));

                    // price_value = cJSON_GetNumberValue(cJSON_GetObjectItem(cJSON_GetArrayItem(results_item, 0), "c"));
                    p = cJSON_GetObjectItem(results_item, "meta");

                    price_value = cJSON_GetNumberValue(cJSON_GetObjectItem(p, "regularMarketPrice"));
                    price_change = cJSON_GetNumberValue(cJSON_GetObjectItem(p, "fulldayChange"));
                    ESP_LOGI(TAG, "Price of %s: %f with change %f\n", ticker_str, price_value, price_change);

                    cJSON_Delete(stock_api_resp);

                    cJSON_AddItemToObject(cJSON_GetObjectItem(res, "new_prices"), ticker_str, cJSON_CreateNumber(price_value));
                    cJSON_AddItemToObject(cJSON_GetObjectItem(res, "price_changes"), ticker_str, cJSON_CreateNumber(price_change));
                    

                    vTaskDelay(pdMS_TO_TICKS(1000));
                }

                if (!successful_api) {
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    break;
                }

                ESP_LOGI(TAG, "Done getting data\n");
                ESP_LOGI(TAG, "Saving day: %s\n", day_current);

                cJSON_AddItemToObject(res, "day", cJSON_CreateString(day_current));
                cJSON_AddItemReferenceToObject(res, "tickers", tickers);

                ESP_LOGI(TAG, "Got new_prices json: %s\n", cJSON_Print(new_prices));

                save_ticker_json(res);

                state = STATE_UPDATE_MARKET_DISPLAY;

                break;

            case STATE_UPDATE_MARKET_DISPLAY:
                ESP_LOGI(TAG, "Updating display\n");
                for (int i = 0; i < cJSON_GetArraySize(tickers); i++) {
                    ticker_str = cJSON_GetStringValue(cJSON_GetArrayItem(tickers, i));

                    ESP_LOGI(
                        TAG,
                        "Adding ticker: %s to message",
                        ticker_str
                    );

                    ui_message.message_type = UI_MSG_MARKET;
                    strncpy(
                        ui_message.market_data.ticker,
                        ticker_str,
                        sizeof(ui_message.market_data.ticker) - 1
                    );

                    new_prices = cJSON_GetObjectItem(res, "new_prices");
                    price_changes = cJSON_GetObjectItem(res, "price_changes");

                    ui_message.market_data.price = cJSON_GetNumberValue(cJSON_GetObjectItem(new_prices, ticker_str));
                    ui_message.market_data.change = cJSON_GetNumberValue(cJSON_GetObjectItem(price_changes, ticker_str));

                    xQueueSend(ui_queue, &ui_message, 0);
                }

                cJSON_Delete(res);

                state = STATE_SLEEP;
                break;

            case STATE_SLEEP:

                ESP_LOGI(TAG, "In state sleep\n");
                
                vTaskDelay(pdMS_TO_TICKS(1000 * 60));
                state = STATE_GET_TIME;
                
                break;
            default:
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

#if !CONFIG_IDF_TARGET_LINUX
    vTaskDelete(NULL);
#endif
}