#include "day_time_handler.h"

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

// returns true if time is after 16:15 (to be safe)
// this is so ugly
bool is_after_close(const char* time_string)
{
    int idx = 0;
    while (idx < strlen(time_string) && time_string[idx] != '.') {
        if (time_string[idx] == 'T' && strlen(time_string) - idx > 4) {
            if (time_string[idx+1] == '1') {
                if (time_string[idx+2] == '6') {
                    if (time_string[idx+4] == '1') {
                        if (time_string[idx+5] >= '5') {
                            return true;
                        }
                    } else if (time_string[idx+4] >= '2') {
                        return true;
                    }
                } else if (time_string[idx+2] >= '7') {
                    return true;
                }
            } else if (time_string[idx+1] == '2') {
                return true;
            }
        }
        idx++;
    }
    return false;
}