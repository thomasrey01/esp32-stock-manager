#pragma once
#ifndef DAY_TIME_HANDLER_H
#define DAY_TIME_HANDLER_H

#include <stdbool.h>
#include <string.h>

#define NUM_TRADING_DAYS 5

extern const char* days[NUM_TRADING_DAYS];

int get_day(const char* day_string);
bool is_after_close(const char* time_string);

#endif