#ifndef TESTS_H
#define TESTS_H
#include "esp_log.h"

void test_time_parse();
void test_display_labels();
void send_init_time();
void send_cpu_stats();
void send_wifi_status();

#endif