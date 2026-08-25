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
#include "driver_st7789.h"
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif

#include "driver_st7789.h"
#include "driver_st7789_font.h"
#include "spi_bridge.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "secrets.h"

#include "esp_http_client.h"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048
static const char *TAG = "HTTP_CLIENT";
static const char *HANDLER_TAG = "HTTP_HANDLER";
#define TICKER "AMZN"

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

static void https_with_hostname_path(void)
{
    char path[256];

    snprintf(path, sizeof(path),
            "/v2/aggs/ticker/%s/prev?adjusted=true&apiKey=%s",
            TICKER,
            API_KEY);

    

    esp_http_client_config_t config = {
        .host = "api.massive.com",
        .path = path,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .event_handler = _http_event_handler,
        .cert_pem = howsmyssl_com_root_cert_pem_start,
        .timeout_ms = 5000,
        .user_data = &response,
    };
    ESP_LOGI(TAG, "HTTPS request with hostname and path =>");
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

// void fill_red(st7789_device_handle_t display)
// {
//     // 170 x 320
//     uint16_t width = 32;
//     uint16_t height = 32;

//     uint16_t *buffer = malloc(width * height * sizeof(uint16_t));

//     if (!buffer) {
//         return;
//     }

//     // Fill framebuffer with black?
//     for (int i = 0; i < width * height; i++) {
//         buffer[i] = 0xFFFF;
//     }

//     for (int i = 0; i < 256; i += width) {
//         for (int j = 0; j < 256; j += height) {
//             st7789_paint(
//             display,
//             buffer,
//             j,
//             j+height,
//             i,
//             i+width
//             );


//         }
        
//     }

//     free(buffer);
// }


static void http_test_task(void *pvParameters)
{
    esp_err_t ret;
    // https_with_hostname_path();
    // const char *ticker_json = "{\"tickers\": [\"AMZN\"]}";
    // ticker_storage_set(ticker_json);
    // ret = ticker_storage_save(ticker_json);
    // ticker_storage_init();


    // ESP_LOGI(TAG, "SPI init result: %s\n", esp_err_to_name(err));


    // st7789_params_t st1178_params = {
    //     .host = SPI2_HOST,
    //     .gpio_cs = GPIO_NUM_22,
    //     .gpio_bckl = GPIO_NUM_23,
    //     .gpio_dc = GPIO_NUM_19,
    // };

    // st7789_device_handle_t st7789_device_handle;
    // st7789_device_handle = st7789_init(&st1178_params);

    // st7789_backlight(st7789_device_handle, 1);

    // st7789_dispon(st7789_device_handle);

    // // vTaskDelay(1000 / portTICK_PERIOD_MS);

    // st7789_swreset(st7789_device_handle);

    st7789_handle_t st_handle = {
        .spi_init = spi_init,
        .spi_deinit = spi_denit,
        .spi_write_cmd = spi_write_cmd,
        .cmd_data_gpio_init = cmd_data_gpio_init,
        .cmd_data_gpio_deinit = cmd_data_gpio_deinit,
        .cmd_data_gpio_write = cmd_data_gpio_write,
        .reset_gpio_init = reset_gpio_init,
        .reset_gpio_deinit = reset_gpio_deinit,
        .reset_gpio_write = reset_gpio_write,
        .debug_print = debug_print,
        .delay_ms = delay_ms,
    };

    uint8_t err = st7789_init(&st_handle);

    if (err != 0) {
        ESP_LOGE(TAG, "ST init failed with value: %d\n", err);
    } else {
        ESP_LOGI(TAG, "ST init successful\n");
    }

    st7789_set_column(&st_handle, 240);
    st7789_set_row(&st_handle, 320);

    gpio_set_level(PIN_NUM_BLK, 1);

    err = st7789_sleep_out(&st_handle);
    ESP_LOGI(TAG, "sleep out: %d", err);

    vTaskDelay(pdMS_TO_TICKS(120));


    err = st7789_set_interface_pixel_format(&st_handle, ST7789_RGB_INTERFACE_COLOR_FORMAT_65K, ST7789_CONTROL_INTERFACE_COLOR_FORMAT_16_BIT);

    ESP_LOGI(TAG, "pixel format: %d", err);

    err = st7789_set_memory_data_access_control(
    &st_handle,
    ST7789_ORDER_COLOR_RGB
    );
    ESP_LOGI(TAG, "MADCTL: %d", err);

    err = st7789_display_on(&st_handle);
    ESP_LOGI(TAG, "display on: %d", err);

    st7789_clear(&st_handle);

    err = st7789_write_string(&st_handle, 42, 32, "Hello", 5, 0x368F, ST7789_FONT_24);
    vTaskDelay(pdMS_TO_TICKS(120));
    err = st7789_write_string(&st_handle, 42, 50, "There", 6, 0x368F, ST7789_FONT_24);
    vTaskDelay(pdMS_TO_TICKS(120));

    // err = st7789_fill_rect(&st_handle, 0, 0, 239, 239, 0xFFFF);
    ESP_LOGI(TAG, "fill rect: %d", err);

    if (err != 0) {
        ESP_LOGE(TAG, "write failed with value: %d\n", err);
    } else {
        ESP_LOGI(TAG, "write successful\n");
    }

    ESP_LOGI(TAG, "Finish http example");
#if !CONFIG_IDF_TARGET_LINUX
    vTaskDelete(NULL);
#endif
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // ESP_ERROR_CHECK(esp_netif_init());
    // ESP_ERROR_CHECK(esp_event_loop_create_default());

    // /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
    //  * Read "Establishing Wi-Fi or Ethernet Connection" section in
    //  * examples/protocols/README.md for more information about this function.
    //  */
    // ESP_ERROR_CHECK(example_connect());
    ESP_LOGI(TAG, "Connected to AP, begin http example");

#if CONFIG_IDF_TARGET_LINUX
    http_test_task(NULL);
#else
    xTaskCreate(&http_test_task, "http_test_task", 8192, NULL, 5, NULL);
#endif
}
