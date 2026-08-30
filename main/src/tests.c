#include "tests.h"
#include "day_time_handler.h"
#include "market.h"
#include "ui.h"

extern QueueSetHandle_t ui_queue;

const char *TAG = "TESTS";

void test_time_parse()
{

    const char * str1 = "2026-08-26T22:44:34.103825+02:00"; // true
    const char * str2 = "2026-08-26T16:44:34.103825+02:00"; // true
    const char * str3 = "2026-08-26T17:01:34.103825+02:00"; // true
    const char * str4 = "2026-08-26T16:01:34.103825+02:00"; // false
    const char * str5 = "2026-08-26T01:44:34.103825+02:00"; // false
    const char* str6 = "T1"; // false

    ESP_LOGI(TAG, "test1: %d\n", is_after_close(str1));
    ESP_LOGI(TAG, "test2: %d\n", is_after_close(str2));
    ESP_LOGI(TAG, "test3: %d\n", is_after_close(str3));
    ESP_LOGI(TAG, "test4: %d\n", is_after_close(str4));
    ESP_LOGI(TAG, "test5: %d\n", is_after_close(str5));
    ESP_LOGI(TAG, "test6: %d\n", is_after_close(str6));
}

void test_display_labels(void)
{
    market_data_t market_data = {0};
    const char *ticker_str1 = "AMZN";
    const char *ticker_str2 = "QBTS";
    const char *ticker_str3 = "GOOG";

    strncpy(
        market_data.ticker,
        ticker_str1,
        sizeof(market_data.ticker) - 1
    );

    market_data.price = 218.11;

    xQueueSend(ui_queue, &market_data, 0);

    strncpy(
        market_data.ticker,
        ticker_str2,
        sizeof(market_data.ticker) - 1
    );

    market_data.price = 14.28;

    xQueueSend(ui_queue, &market_data, 0);

    strncpy(
        market_data.ticker,
        ticker_str3,
        sizeof(market_data.ticker) - 1
    );

    market_data.price = 1073.92;

    xQueueSend(ui_queue, &market_data, 0);
    
}