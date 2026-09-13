#ifndef MESSAGES_H
#define MESSAGES_H

typedef struct {
    int hour;
    int minute;
    int second;
} clock_data_t;

typedef struct {
    char ticker[16];
    float price;
    float change;
    float percent;
} market_data_t;

typedef enum {
    UI_MSG_CLOCK,
    UI_MSG_MARKET
} ui_message_type_t;

typedef struct ui_message {
    ui_message_type_t message_type;

    union {
        clock_data_t clock_data;

        market_data_t market_data;
    };
} ui_message_t;

#endif