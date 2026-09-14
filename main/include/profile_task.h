#ifndef PROFILE_TASK_H
#define PROFILE_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "messages.h"


void profile_task(void *pvParameters);

#endif