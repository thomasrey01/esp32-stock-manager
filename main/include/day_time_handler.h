#pragma once
#ifndef DAY_TIME_HANDLER_H
#define DAY_TIME_HANDLER_H

#include <stdbool.h>
#include <string.h>
#include "time_task.h"
#include "esp_system.h"
#include "esp_log.h"


#define NUM_TRADING_DAYS 5

extern const char* days[NUM_TRADING_DAYS];

int get_day(const char* day_string);
bool is_after_close(clock_data_t clock_data);
esp_err_t parse_time(const char* time_string, clock_data_t* clock_data);

#endif