#ifndef WIFI_SIGNAL_TASK_H
#define WIFI_SIGNAL_TASK_H

#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "messages.h"

void wifi_signal_task(void *pvParameters);

#endif