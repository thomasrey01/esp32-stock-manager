#include "time_task.h"

#include "sys/time.h"

void set_system_time(time_t timestamp)
{
    struct timeval tv = {
        .tv_sec = timestamp,
        .tv_usec = 0
    };

    settimeofday(&tv, NULL);
}

