#include "time_task.h"

#include "sys/time.h"

QueueSetHandle_t time_queue;
extern QueueSetHandle_t ui_queue;

static const char* TAG = "TIME_TASK";

void set_system_time(time_t timestamp)
{
    struct timeval tv = {
        .tv_sec = timestamp,
        .tv_usec = 0
    };

    settimeofday(&tv, NULL);
}

void time_task(void *pvParameters)
{
    TickType_t last_wake = xTaskGetTickCount();
    ui_message_t ui_message = {
        .message_type = UI_MSG_CLOCK,
    };
    clock_data_t current_time = {
        .hour = 0,
        .minute = 0,
        .second = 0,
    };

    for (;;) {
        xQueueReceive(time_queue, &current_time, 0);
        current_time.second += 1;
        if (current_time.second >= 60) {
            current_time.minute += 1;
            current_time.second = 0;
            if (current_time.minute >= 60) {
                current_time.hour += 1;
                current_time.minute = 0;
                if (current_time.hour >= 24) {
                    current_time.hour = 0;
                }
            }
        }

        ui_message.clock_data = current_time;
        xQueueSend(ui_queue, &ui_message, 0);
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(1000));

    }
}
