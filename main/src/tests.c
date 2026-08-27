#include "tests.h"
#include "day_time_handler.h"

void test_time_parse()
{

    const char * str1 = "2026-08-26T22:44:34.103825+02:00"; // true
    const char * str2 = "2026-08-26T16:44:34.103825+02:00"; // true
    const char * str3 = "2026-08-26T17:01:34.103825+02:00"; // true
    const char * str4 = "2026-08-26T16:01:34.103825+02:00"; // false
    const char * str5 = "2026-08-26T01:44:34.103825+02:00"; // false
    const char* str6 = "T1"; // false

    ESP_LOGI("TESTS", "test1: %d\n", is_after_close(str1));
    ESP_LOGI("TESTS", "test2: %d\n", is_after_close(str2));
    ESP_LOGI("TESTS", "test3: %d\n", is_after_close(str3));
    ESP_LOGI("TESTS", "test4: %d\n", is_after_close(str4));
    ESP_LOGI("TESTS", "test5: %d\n", is_after_close(str5));
    ESP_LOGI("TESTS", "test6: %d\n", is_after_close(str6));
}