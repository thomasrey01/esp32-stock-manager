#include "http.h"

static const char *TAG = "HTTP_CLIENT";
static const char *HANDLER_TAG = "HTTP_HANDLER";

char recv_buf[MAX_HTTP_RECV_BUFFER] = {0};

http_response_t response = {
    .buffer = recv_buf,
    .length = 0,
    .max_length = sizeof(recv_buf)
};

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

esp_err_t https_with_hostname_path(const char* host, const char *path)
{

    // snprintf(path, sizeof(path),
    //         "/v2/aggs/ticker/%s/prev?adjusted=true&apiKey=%s",
    //         TICKER,
    //         API_KEY);

    // url is: api.massive.com
    
    response.length = 0;
    response.buffer[0] = '\0';

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

    return err;
}