#ifndef MARKET_TASK_H
#define MARKET_TASK_H

#include "http.h"
#include "cJSON.h"
#include "ui.h"
#include "secrets.h"
#include "ticker_storage.h"
#include "day_time_handler.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

void market_task(void *pvParameters);

#endif