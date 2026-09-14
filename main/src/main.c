/* ESP HTTP Client Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <sys/param.h>
#include <stdlib.h>
#include <ctype.h>
#include "esp_log.h"
#include "nvs_flash.h"

#include "cJSON.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "secrets.h"
#include "ticker_storage.h"

#include "market_task.h"
#include "profile_task.h"
#include "wifi_signal_task.h"

#include "ui.h"
#include "http.h"

#include "day_time_handler.h"
#include "tests.h"

#define DEBUG_NO_WIFI 0
#define DEBUG_NO_PROFILER 0

static const char *TAG = "MAIN";


extern QueueSetHandle_t ui_queue;
extern QueueSetHandle_t time_queue;

void app_main(void)
{
    char ip_addr_str[20];

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ui_init();
    ui_queue = xQueueCreate(10, sizeof(ui_message_t));
    time_queue = xQueueCreate(1, sizeof(clock_data_t));

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
    //  * Read "Establishing Wi-Fi or Ethernet Connection" section in
    //  * examples/protocols/README.md for more information about this function.
    //  */

    #if DEBUG_NO_WIFI
        snprintf(ip_addr_str, sizeof(ip_addr_str), "127.0.0.1");
    #else
    ESP_ERROR_CHECK(example_connect());
    ESP_LOGI(TAG, "Connected to AP, begin http example");

    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

    if (netif == NULL) {
        ESP_LOGE(TAG, "Could not get Wi-Fi STA netif");
        return;
    }

    esp_netif_ip_info_t ip;
    ESP_ERROR_CHECK(esp_netif_get_ip_info(netif, &ip));
    snprintf(ip_addr_str, sizeof(ip_addr_str), IPSTR, IP2STR(&ip.ip));

    #endif
    ESP_LOGI(TAG, "ip_addr_str: %s\n", ip_addr_str);
    ui_wifi_ready(ip_addr_str);

#if CONFIG_IDF_TARGET_LINUX
    http_test_task(NULL);
#else

    #if !DEBUG_NO_WIFI
    xTaskCreate(&market_task, "market_task", 8192, NULL, 5, NULL);
    xTaskCreate(&wifi_signal_task, "wifi_signal_task", 2048, NULL, 5, NULL);
    #else
    test_display_labels();
    send_init_time();    
    // ui_test_colors();
    #endif
    xTaskCreate(&lvgl_task, "lvgl_task", 8192, NULL, 5, NULL);
    xTaskCreate(&time_task, "clock_task", 8192, NULL, 6, NULL);

    #if DEBUG_NO_PROFILER
    send_cpu_stats();
    #else
    xTaskCreate(&profile_task, "profile_task", 4096, NULL, 7, NULL);
    #endif

    #if DEBUG_NO_WIFI
    send_wifi_status();

    #endif
#endif

}
