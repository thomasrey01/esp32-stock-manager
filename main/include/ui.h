#pragma once

#ifndef UI_H
#define UI_H

#include "esp_err.h"

esp_err_t ui_init(void);
void set_num_tickers(int num);
void ui_wifi_ready(const char *address);
void lvgl_task(void *pvParameters);

#endif

