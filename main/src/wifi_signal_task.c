#include "wifi_signal_task.h"

static const char* TAG = "WIFI_SIGNAL_TASK";

extern QueueSetHandle_t ui_queue;
extern EventGroupHandle_t wifi_event_group;
int disconnect_count = 0;

static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{

    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START) {

        ESP_LOGI(TAG, "WiFi started\n");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT &&
                event_id == WIFI_EVENT_STA_DISCONNECTED) {
        
        ESP_LOGW(TAG, "WiFi disconnected, count: %d", disconnect_count);

        vTaskDelay(pdMS_TO_TICKS(disconnect_count * 500));
        disconnect_count++;

        xEventGroupClearBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT
        );

        esp_wifi_connect();
    } else if (event_base == IP_EVENT &&
                event_id == IP_EVENT_STA_GOT_IP) {
        
        
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        ESP_LOGI(
            TAG,
            "Got IP: " IPSTR,
            IP2STR(&event->ip_info.ip)
        );

        xEventGroupSetBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT
        );
        
        ui_message_t ui_message = {
            .message_type = UI_MSG_WIFI_CONNECT,
        };

        snprintf(
            ui_message.ip_addr,
            sizeof(ui_message.ip_addr),
            IPSTR,
            IP2STR(&event->ip_info.ip)
        );

        xQueueSend(ui_queue, &ui_message, 0);

        disconnect_count = 0;
    }
}

void wifi_signal_task(void *pvParameters)
{


    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());



    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(
        esp_wifi_init(&cfg)
    );

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL
        )
    );

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &wifi_event_handler,
            NULL
        )
    );

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_EXAMPLE_WIFI_SSID,
            .password = CONFIG_EXAMPLE_WIFI_PASSWORD
        }
    };

    ESP_ERROR_CHECK(
        esp_wifi_set_mode(WIFI_MODE_STA)
    );

    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config
        )
    );

    ESP_ERROR_CHECK(
        esp_wifi_start()
    );

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