#include "tests.h"
#include "day_time_handler.h"
#include "messages.h"
#include "ui.h"
#include "time_task.h"

extern QueueSetHandle_t ui_queue;
extern QueueSetHandle_t time_queue;

const char *TAG = "TESTS";

void test_time_parse()
{

    clock_data_t clock_data;

    const char * str1 = "22:44:34.103825+02:00"; // true
    const char * str2 = "16:44:34.103825+02:00"; // true
    const char * str3 = "17:01:34.103825+02:00"; // true
    const char * str4 = "2026-08-26T16:01:34.103825+02:00"; // false
    const char * str5 = "2026-08-26T01:44:34.103825+02:00"; // false
    const char* str6 = "T1"; // false


    parse_time(str1, &clock_data);

    ESP_LOGI(TAG, "Current time: %d:%d:%d\n", 
                clock_data.hour, 
                clock_data.minute, 
                clock_data.second
        );

    ESP_LOGI(TAG, "test1: %d\n", is_after_close(clock_data));
    parse_time(str2, &clock_data);

    ESP_LOGI(TAG, "Current time: %d:%d:%d\n", 
                clock_data.hour, 
                clock_data.minute, 
                clock_data.second
        );

    ESP_LOGI(TAG, "test2: %d\n", is_after_close(clock_data));
    parse_time(str3, &clock_data);
    ESP_LOGI(TAG, "test3: %d\n", is_after_close(clock_data));
    parse_time(str4, &clock_data);
    ESP_LOGI(TAG, "test4: %d\n", is_after_close(clock_data));
    parse_time(str5, &clock_data);
    ESP_LOGI(TAG, "test5: %d\n", is_after_close(clock_data));
    parse_time(str6, &clock_data);
    ESP_LOGI(TAG, "test6: %d\n", is_after_close(clock_data));
}

void test_display_labels()
{
    ui_message_t ui_message = {0};
    const char *ticker_str1 = "AMZN";
    const char *ticker_str2 = "QBTS";
    const char *ticker_str3 = "GOOG";

    ui_message.message_type = UI_MSG_MARKET;

    strncpy(
        ui_message.market_data.ticker,
        ticker_str1,
        sizeof(ui_message.market_data.ticker) - 1
    );

    ui_message.market_data.price = 218.11;

    xQueueSend(ui_queue, &ui_message, 0);

    strncpy(
        ui_message.market_data.ticker,
        ticker_str2,
        sizeof(ui_message.market_data.ticker) - 1
    );

    ui_message.market_data.price = 14.28;

    xQueueSend(ui_queue, &ui_message, 0);

    strncpy(
        ui_message.market_data.ticker,
        ticker_str3,
        sizeof(ui_message.market_data.ticker) - 1
    );

    ui_message.market_data.price = 1073.92;

    xQueueSend(ui_queue, &ui_message, 0);

    ESP_LOGI(TAG, "display test done\n");
    
}

void send_init_time()
{
    clock_data_t clock_data = {
        .hour = 16,
        .minute = 20,
        .second = 10
    };


    xQueueSend(time_queue, &clock_data, 0);
}