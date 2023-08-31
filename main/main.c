
#include <sys/param.h>
#include "platform_wifi_client.h"
#include <strings.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_mac.h"

#include "nvs_flash.h"
#include "esp_system.h"


#include "esp_http_server.h"
#include "lwip/err.h"
#include "lwip/sys.h"

static const char *TAG = "Sensor Server";

extern const char root_start[] asm("_binary_root_html_start");
extern const char root_end[] asm("_binary_root_html_end");

extern const char success_start[] asm("_binary_success_html_start");
extern const char success_end[] asm("_binary_success_html_end");

// HTTP GET Handler
static esp_err_t root_get_handler(httpd_req_t *req)
{
    const uint32_t root_len = root_end - root_start;

    ESP_LOGI(TAG, "Serve root");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, root_start, root_len);

    return ESP_OK;
}

static esp_err_t success_get_handler(httpd_req_t *req)
{
    const uint32_t success_len = success_end - success_start;

    ESP_LOGI(TAG, "Success page");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, success_start, success_len);

    return ESP_OK;
}

static const httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler
};

static const httpd_uri_t success_page = {
    .uri = "/success",
    .method = HTTP_GET,
    .handler = success_get_handler
};

// HTTP Error (404) Handler - Redirects all requests to the root page
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    // Set status
    httpd_resp_set_status(req, "302 Temporary Redirect");
    // Redirect to the "/" root directory
    httpd_resp_set_hdr(req, "Location", "/");
    // iOS requires content in the response to detect a captive portal, simply redirecting is not sufficient.
    httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);

    ESP_LOGI(TAG, "Redirecting to root");
    return ESP_OK;
}

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets = 7;
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &success_page);
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
    }
    return server;
}

void app_main(void)
{
    /*
        Turn of warnings from HTTP server as redirecting traffic will yield
        lots of invalid requests
    */
   platform_wifi_cfg_t wifi_cfg = {
    .ssid = "insert_ssid",
    .password = "insert_pwd",
    .conn_attempts = 5,
   };

    esp_log_level_set("httpd_uri", ESP_LOG_ERROR);
    esp_log_level_set("httpd_txrx", ESP_LOG_ERROR);
    esp_log_level_set("httpd_parse", ESP_LOG_ERROR);
    // Initialize NVS needed by Wi-Fi
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_client_init();
    wifi_client_configure(&wifi_cfg);
    wifi_client_connect();
    // Start the server for the first time
    static httpd_handle_t server = NULL;
    server = start_webserver();
    // Start the DNS server that will redirect all queries to the softAP IP
    while(server) {
        sleep(5);
    }
}
