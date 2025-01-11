/*******************************************************************
 * File Name         : https_server.c
 * Description       : Creates a HTTPS server to display the data recieved by the udp server
 *
 * Author            : Noah Plank
 *
 ******************************************************************/

#include "https_server.h"

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "esp_eth.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <esp_https_server.h>
#include "esp_tls.h"
#include "sdkconfig.h"

#include "udp_server.h"

static const char *TAG = "HTTPS Server";

static httpd_handle_t server = NULL;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == ESP_HTTPS_SERVER_EVENT)
    {
        if (event_id == HTTPS_SERVER_EVENT_ERROR)
        {
            esp_https_server_last_error_t *last_error = (esp_tls_last_error_t *)event_data;
            ESP_LOGE(TAG, "Error event triggered: last_error = %s, last_tls_err = %d, tls_flag = %d", esp_err_to_name(last_error->last_error), last_error->esp_tls_error_code, last_error->esp_tls_flags);
        }
    }
}

// Root URI handler
static esp_err_t root_get_handler(httpd_req_t *req)
{
    extern const unsigned char start_page_start[] asm("_binary_start_page_html_start");
    extern const unsigned char start_page_end[] asm("_binary_start_page_html_end");
    const size_t start_page_html_len = start_page_end - start_page_start;

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)start_page_start, start_page_html_len);

    return ESP_OK;
}

static const httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler};

// Data URI handler
static esp_err_t data_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    char json_response[512];

    char readable_time[80] = {0};

    udp_data_t udp_data = get_udp_data();

    if (0 == udp_data.wakeup_time_in_unix)
    {
        snprintf(readable_time, sizeof(readable_time), "No data received yet");
    }
    else
    {
        strftime(readable_time, sizeof(readable_time), "%Y-%m-%d %H:%M:%S", localtime(&udp_data.wakeup_time_in_unix));
    }

    snprintf(json_response, sizeof(json_response),
             "{\"wakeup_time\": \"%s\", \"battery_charge_in_perc\": %" PRIu8 ", \"scale_weight_in_g\": %" PRIu32 ", \"message_buffer\": \"%s\"}",
             readable_time, udp_data.battery_charge_in_perc, udp_data.scale_weight_in_g, udp_data.message_buffer);

    printf("\njson_response = \n%s\n", json_response);
    httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

static const httpd_uri_t data = {
    .uri = "/data",
    .method = HTTP_GET,
    .handler = data_get_handler};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;

    ESP_LOGI(TAG, "Starting server");

    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();

    // The certificates in the cert folder are included as binary files in the build (see CMakeLists.txt)
    // the linker generates following symbols "_binary_filename_pem_start"  and "_binary_servercert_pem_end"
    // which can be used to get the start and end of the binary data with the assembly directive "asm"
    extern const unsigned char servercert_start[] asm("_binary_certificate_pem_start");
    extern const unsigned char servercert_end[] asm("_binary_certificate_pem_end");
    conf.servercert = servercert_start;
    conf.servercert_len = servercert_end - servercert_start;

    extern const unsigned char prvtkey_pem_start[] asm("_binary_private_key_no_passphrase_pem_start");
    extern const unsigned char prvtkey_pem_end[] asm("_binary_private_key_no_passphrase_pem_end");
    conf.prvtkey_pem = prvtkey_pem_start;
    conf.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

    esp_err_t ret = httpd_ssl_start(&server, &conf);
    if (ESP_OK != ret)
    {
        ESP_LOGE(TAG, "Error starting server!");
        return NULL;
    }

    // Set URI handlers
    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &root);
    httpd_register_uri_handler(server, &data);

    ESP_ERROR_CHECK(esp_event_handler_register(ESP_HTTPS_SERVER_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    return server;
}

void start_https_server(void)
{
    server = start_webserver();
}
