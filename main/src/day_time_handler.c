#include "day_time_handler.h"

static const char *TAG = "DAY_TIME_HANDLER";

const char* days[NUM_TRADING_DAYS] = {
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
};

int get_day(const char* day_string)
{
    for (int i = 0; i < NUM_TRADING_DAYS; i++) {
        if (strcmp(day_string, days[i]) == 0) return i;
    }
    return -1;
}

esp_err_t parse_time(const char* time_string, clock_data_t *clock_data)
{
    if (strlen(time_string) < 9) {
        return ESP_FAIL;
    }

    if (time_string[2] != ':' || time_string[5] != ':') {
        ESP_LOGE(TAG, "Wrong time format: %s", time_string);
        return ESP_FAIL;
    }

    clock_data->hour = (int)(time_string[0] - '0') * 10 + (int)(time_string[1] - '0');
    clock_data->minute = (int)(time_string[3] - '0') * 10 + (int)(time_string[4] - '0');
    clock_data->second = (int)(time_string[6] - '0') * 10 + (int)(time_string[7] - '0');

    return ESP_OK;
    
}

bool is_after_close(clock_data_t clock_data)
{
        return (
            clock_data.hour >= 16 &&
            clock_data.minute >= 15
        );
}