#ifndef MARKET_H
#define MARKET_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct {
    char ticker[16];
    float price;
    float change;
    float percent;
} market_data_t;

QueueSetHandle_t ui_queue;

#endif