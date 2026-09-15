#ifndef WIFI_SIGNAL_TASK_H
#define WIFI_SIGNAL_TASK_H

#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_system.h"
#include "messages.h"

#include "esp_event.h"
#include "esp_netif.h"

#define WIFI_CONNECTED_BIT BIT0

void wifi_signal_task(void *pvParameters);

#endif