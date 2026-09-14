#ifndef MESSAGES_H
#define MESSAGES_H

typedef struct {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} clock_data_t;

typedef struct {
    char ticker[16];
    float price;
    float change;
    float percent;
} market_data_t;

typedef struct {
    uint8_t cpu0;
    uint8_t cpu1;
    uint8_t avg;
} cpu_stats_t;

typedef enum {
    UI_MSG_CLOCK,
    UI_MSG_MARKET,
    UI_MSG_CPU,
    UI_MSG_WIFI_STATUS,
} ui_message_type_t;

typedef struct ui_message {
    ui_message_type_t message_type;

    union {
        clock_data_t clock_data;
        cpu_stats_t cpu_stats;
        market_data_t market_data;
        int8_t wifi_data;
    };
} ui_message_t;

#endif