#ifndef TIME_TASK_H 
#define TIME_TASK_H

typedef struct {
    int hour;
    int minute;
    int second;
} clock_data_t;

void time_task(void);

#endif