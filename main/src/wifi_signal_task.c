#include "wifi_signal_task.h"

static const char* TAG = "WIFI_SIGNAL_TASK";

extern QueueSetHandle_t ui_queue;

void wifi_signal_task(void *pvParameters)
{
    wifi_ap_record_t ap_info;
    ui_message_t ui_message = {
        .message_type = UI_MSG_WIFI_STATUS
    };

    for (;;) {
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            ui_message.wifi_data = ap_info.rssi;
            ESP_LOGI(
                TAG,
                "Got wifi strength of %d: \n",
                ap_info.rssi
            );

        } else {
            ui_message.wifi_data = 1;
        }

        xQueueSend(ui_queue, &ui_message, 0);


        vTaskDelay(pdMS_TO_TICKS(500));
    }
}