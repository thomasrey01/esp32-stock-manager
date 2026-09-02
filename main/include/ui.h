#pragma once

#ifndef UI_H
#define UI_H

#include "esp_err.h"
#include "market.h"
#include "time_task.h"

typedef enum {
    UI_MSG_CLOCK,
    UI_MSG_MARKET
} ui_message_type_t;

typedef struct ui_message {
    ui_message_type_t message_type;

    union {
        clock_data_t clock_data;

        market_data_t market_data;
    };
} ui_message_t;

esp_err_t ui_init(void);
void set_num_tickers(int num);
void ui_wifi_ready(const char *address);
void lvgl_task(void *pvParameters);

#endif

