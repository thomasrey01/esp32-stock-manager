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
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "protocol_examples_utils.h"
#include "esp_tls.h"
#include "cJSON.h"
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "secrets.h"
#include "ticker_storage.h"

#include "esp_http_client.h"
#include "esp_crt_bundle.h"

#include "ui.h"

#include "day_time_handler.h"
#include "tests.h"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048
static const char *TAG = "HTTP_CLIENT";
static const char *HANDLER_TAG = "HTTP_HANDLER";
const char *ticker_json = "{\"tickers\": [\"AMZN\", \"QBTS\"]}";

#define ISDEBUG_FIRST 1

typedef enum {
    STATE_FIRST_TIME_INIT,
    STATE_INIT,
    STATE_GET_TIME,
    STATE_CHECK_MARKET,
    STATE_SAVE_NVS,
    STATE_UPDATE_DISPLAY,
    STATE_SLEEP,
} market_state_t;

char recv_buf[MAX_HTTP_RECV_BUFFER] = {0};

typedef struct {
    char *buffer;
    int length;
    int max_length;
} http_response_t;

http_response_t response = {
    .buffer = recv_buf,
    .length = 0,
    .max_length = sizeof(recv_buf)
};

extern QueueSetHandle_t ui_queue;

/* Root cert for howsmyssl.com, taken from howsmyssl_com_root_cert.pem

   The PEM file was extracted from the output of this command:
   openssl s_client -showcerts -connect www.howsmyssl.com:443 </dev/null

   The CA root cert is the last cert given in the chain of certs.

   To embed it in the app binary, the PEM file is named
   in the component.mk COMPONENT_EMBED_TXTFILES variable.
*/
extern const char howsmyssl_com_root_cert_pem_start[] asm("_binary_howsmyssl_com_root_cert_pem_start");
extern const char howsmyssl_com_root_cert_pem_end[]   asm("_binary_howsmyssl_com_root_cert_pem_end");

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len = 0;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_HEADERS_COMPLETE:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADERS_COMPLETE");
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(HANDLER_TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);

            http_response_t *response = evt->user_data;

            ESP_LOGI(HANDLER_TAG, "DATA LEN: %d, CURRENT: %d", evt->data_len, response->length);

            if (response->length + evt->data_len < response->max_length) {

                memcpy(response->buffer + response->length,
                        evt->data,
                        evt->data_len);

                response->length += evt->data_len;
                response->buffer[response->length] = '\0';
            }


            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
#if CONFIG_EXAMPLE_ENABLE_RESPONSE_BUFFER_DUMP
                ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
#endif
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
        default:
            break;
    }
    return ESP_OK;
}

static void https_with_hostname_path(const char* host, const char *path)
{

    // snprintf(path, sizeof(path),
    //         "/v2/aggs/ticker/%s/prev?adjusted=true&apiKey=%s",
    //         TICKER,
    //         API_KEY);

    // url is: api.massive.com

    esp_http_client_config_t config = {
        .host = host,
        .path = path,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .event_handler = _http_event_handler,
        .cert_pem = howsmyssl_com_root_cert_pem_start,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 5000,
        .user_data = &response,
    };
    ESP_LOGI(TAG, "HTTPS request with hostname and path => %s%s", host, path);
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));



        ESP_LOGI(TAG, "%s\n", response.buffer);
                
    } else {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}


/*
 *  http_native_request() demonstrates use of low level APIs to connect to a server,
 *  make a http request and read response. Event handler is not used in this case.
 *  Note: This approach should only be used in case use of low level APIs is required.
 *  The easiest way is to use esp_http_perform()
 */


// Returns the json of ticker price
/*
    {"day": "DAY", "tickers": ["TICKER1", ..."], "prices": {"TICKER1": ...}}
    right now this is only getting the ticker lists...
*/
cJSON* get_ticker_json()
{
    esp_err_t err;
    cJSON *j = NULL;

    size_t len = TICKER_MAX_BUF;

    err = ticker_storage_load(&len, TICKERS);

    const char *ticker_str = ticker_storage_get(TICKERS);

    ESP_LOGI(TAG, "Got ticker string: %s\n", ticker_str);

    j = cJSON_Parse(ticker_str);

    ESP_LOGI(TAG, "JSON string: %s\n", cJSON_Print(j));

    cJSON *tickers_item = cJSON_GetObjectItem(j, "tickers");


    // ticker_storage_set(ticker_json, TICKERS);
    // err = ticker_storage_save(TICKERS);

    return j;
}

esp_err_t save_ticker_json(cJSON *j)
{
    esp_err_t err;

    err = ticker_storage_set(cJSON_Print(j), PRICES);

    ESP_ERROR_CHECK(err);

    err = ticker_storage_save(PRICES);

    return err;

}

static void market_task(void *pvParameters)
{
    static market_state_t state = STATE_INIT;
    char api_path[256];
    cJSON *j = NULL;
    char *day = NULL;
    cJSON *tickers = NULL;
    cJSON *old_prices;
    cJSON *new_prices = NULL;
    cJSON *time_api_resp;
    cJSON *stock_api_resp;
    char *day_current = NULL;
    char *time_current = NULL;
    char * ticker_str = NULL;
    
    cJSON *results_item;
    double price_value;

    for (;;) {
        switch(state) {

            case STATE_INIT:

                ESP_LOGI(TAG, "In INIT\n");

                ticker_storage_init();
                
                j = get_ticker_json(); // fix this
                day = cJSON_GetStringValue(cJSON_GetObjectItem(j, "day"));
                tickers = cJSON_GetObjectItem(j, "tickers");
                old_prices = cJSON_GetObjectItem(j, "prices");

                state = STATE_GET_TIME;
                break;
            case STATE_GET_TIME:
                ESP_LOGI(TAG, "In GET TIME\n");
                // First get current day
                https_with_hostname_path("timeapi.io", "/api/v1/timezone/zone?timeZone=America\%2FNew_York");

                ESP_LOGI(TAG, "Got time api response: %s\n", response.buffer);
                time_api_resp = cJSON_ParseWithLength(response.buffer, response.length);
                ESP_LOGI(TAG, "loaded json with length %d\n", response.length);
                strcpy(
                    day_current,
                    cJSON_GetStringValue(cJSON_GetObjectItem(time_api_resp, "day_of_week"))
                );
                ESP_LOGI(TAG, "Current day: %s\n", day_current);
                time_current = cJSON_GetStringValue(cJSON_GetObjectItem(time_api_resp, "local_time"));
                ESP_LOGI(TAG, "Current time: %s\n", time_current);

                if (
                    day == NULL || ISDEBUG_FIRST ||
                    (
                        get_day(day) < get_day(day_current) &&
                        is_after_close(time_current)
                    )
                ) {
                    state = STATE_CHECK_MARKET;
                } else {
                    state = STATE_SLEEP;
                }

                cJSON_Delete(time_api_resp);

                break;

            case STATE_CHECK_MARKET:

                ESP_LOGI(TAG, "In check market\n");

                new_prices = cJSON_CreateObject();
                

                for (int i = 0; i < cJSON_GetArraySize(tickers); i++) {

                    ticker_str = cJSON_GetStringValue(cJSON_GetArrayItem(tickers, i));

                    ESP_LOGI(TAG, "Getting ticker: %s\n", cJSON_Print(cJSON_GetArrayItem(tickers, i)));
                    
                    snprintf(api_path, sizeof(api_path),
                        "/v2/aggs/ticker/%s/prev?adjusted=true&apiKey=%s",
                        ticker_str,
                        API_KEY
                    );

                    ESP_LOGI(TAG, "getting path: %s\n", api_path);

                    response.length = 0;
                    response.buffer[0] = '\0';

                    https_with_hostname_path("api.massive.com", api_path);

                    stock_api_resp = cJSON_ParseWithLength(response.buffer, response.length);

                    results_item = cJSON_GetObjectItem(stock_api_resp, "results");

                    price_value = cJSON_GetNumberValue(cJSON_GetObjectItem(cJSON_GetArrayItem(results_item, 0), "c"));

                    ESP_LOGI(TAG, "Price of %s: %f\n", ticker_str, price_value);

                    cJSON_Delete(stock_api_resp);

                    cJSON_AddItemToObject(new_prices, ticker_str, cJSON_CreateNumber(price_value));

                    vTaskDelay(pdMS_TO_TICKS(200));
                }

                ESP_LOGI(TAG, "Done getting data\n");
                ESP_LOGI(TAG, "Saving day: %s\n", day_current);

                cJSON_AddItemToObject(new_prices, "day", cJSON_CreateString(day_current));
                cJSON_AddItemToObject(new_prices, "tickers", tickers);

                ESP_LOGI(TAG, "Got new_prices json: %s\n", cJSON_Print(new_prices));

                save_ticker_json(new_prices);

                state = STATE_UPDATE_DISPLAY;

                
                break;

            case STATE_UPDATE_DISPLAY:
                ESP_LOGI(TAG, "Updating display\n");
                market_data_t market_data = {0};
                for (int i = 0; i < cJSON_GetArraySize(tickers); i++) {
                    ticker_str = cJSON_GetStringValue(cJSON_GetArrayItem(tickers, i));
                    strncpy(
                        market_data.ticker,
                        ticker_str,
                        sizeof(market_data.ticker) - 1
                    );

                    market_data.price = cJSON_GetNumberValue(cJSON_GetObjectItem(new_prices, ticker_str));

                    xQueueSend(ui_queue, &market_data, 0);
                }

                state = STATE_SLEEP;
                break;

            case STATE_SLEEP:

                ESP_LOGI(TAG, "In state sleep\n");
                
                vTaskDelay(pdMS_TO_TICKS(1000 * 60));
                state = STATE_GET_TIME;
                
                break;
            default:
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }

#if !CONFIG_IDF_TARGET_LINUX
    vTaskDelete(NULL);
#endif
}

void app_main(void)
{
    char ip_addr_str[17];

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ui_init();
    ui_queue = xQueueCreate(5, sizeof(market_data_t));

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
    //  * Read "Establishing Wi-Fi or Ethernet Connection" section in
    //  * examples/protocols/README.md for more information about this function.
    //  */
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
    ESP_LOGI(TAG, "ip_addr_str: %s\n", ip_addr_str);
    ui_wifi_ready(ip_addr_str);
#if CONFIG_IDF_TARGET_LINUX
    http_test_task(NULL);
#else
    // xTaskCreate(&market_task, "market_task", 8192, NULL, 5, NULL);
    xTaskCreate(&lvgl_task, "lvgl_task", 4096, NULL, 5, NULL);
#endif

    test_display_labels();

}
