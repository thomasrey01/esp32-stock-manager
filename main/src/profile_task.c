#include "profile_task.h"

static const char* TAG = "PROFILER";

extern QueueSetHandle_t ui_queue;

void profile_task(void *pvParameters)
{
    TaskStatus_t tasks[20];

    uint32_t previous_total = 0;
    uint32_t previous_idle0 = 0;
    uint32_t previous_idle1 = 0;

    ui_message_t ui_message = {
        .message_type = UI_MSG_CPU
    };

    cpu_stats_t cpu_stats = {0};

    for (;;) {

        vTaskDelay(pdMS_TO_TICKS(1000));

        uint32_t total_runtime;

        UBaseType_t task_count =
            uxTaskGetSystemState(
                tasks,
                20,
                &total_runtime
            );

        uint32_t idle0 = 0;
        uint32_t idle1 = 0;

        for (int i = 0; i < task_count; i++) {

            if (strcmp(tasks[i].pcTaskName, "IDLE0") == 0) {
                idle0 = tasks[i].ulRunTimeCounter;
            }

            if (strcmp(tasks[i].pcTaskName, "IDLE1") == 0) {
                idle1 = tasks[i].ulRunTimeCounter;
            }
        }

        if (previous_total != 0) {

            uint32_t delta_total =
                total_runtime - previous_total;

            uint32_t delta_idle0 =
                idle0 - previous_idle0;

            uint32_t delta_idle1 =
                idle1 - previous_idle1;

            uint32_t cpu0 =
                100 - ((delta_idle0 * 100ULL) / delta_total);

            uint32_t cpu1 =
                100 - ((delta_idle1 * 100ULL) / delta_total);

            uint32_t cpu_average =
                (cpu0 + cpu1) / 2;

            ESP_LOGI(
                TAG,
                "CPU0: %lu%% CPU1: %lu%% AVG: %lu%%",
                cpu0,
                cpu1,
                cpu_average
            );
            cpu_stats.cpu0 = cpu0;
            cpu_stats.cpu1 = cpu1;
            cpu_stats.avg = cpu_average;

            ui_message.cpu_stats = cpu_stats;

            xQueueSend(ui_queue, &ui_message, 0);

        }

        previous_total = total_runtime;
        previous_idle0 = idle0;
        previous_idle1 = idle1;
    }
}