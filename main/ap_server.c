#include "ap_server.h"
#include "esp_http_server.h"

// FreeRtos Libraries
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"

// Platform Libraries
#include "controller.h"

static const char *TAG = "AP Server";

extern const char root_start[] asm("_binary_root_html_start");
extern const char root_end[] asm("_binary_root_html_end");
static QueueHandle_t s_queue;

static httpd_handle_t server = NULL;
static char s_ssid[32];
static char s_pwd[32];
typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[5120];
} rest_server_context_t;

static void send_wifi_credentials(void)
{
    uint8_t msg[72];
    msg[0] = WIFI_DATA;
    memcpy(&msg[1], s_ssid, 32);
    memcpy(&msg[33], s_pwd, 32);
    xQueueSend(s_queue, msg, 0);
}

// HTTP GET Handler
static esp_err_t root_get_handler(httpd_req_t *req)
{
    const uint32_t root_len = root_end - root_start;

    ESP_LOGI(TAG, "Serve root");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, root_start, root_len);

    return ESP_OK;
}

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

static esp_err_t auth_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= 5120) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';
    cJSON *root = cJSON_Parse(buf);


    char* in_ssid = cJSON_GetObjectItem(root, "ssid")->valuestring;
    char* in_pwd = cJSON_GetObjectItem(root, "password")->valuestring;
    memset(s_ssid, 0, 32);
    memset(s_pwd, 0, 32);
    memcpy(s_ssid, in_ssid, strlen(in_ssid));
    memcpy(s_pwd, in_pwd, strlen(in_pwd));
    cJSON_Delete(root);
    send_wifi_credentials();
    // Needs to send ssid and pwd over to controller
    return ESP_OK;
}

void start_ap_server(QueueHandle_t queue)
{
    s_queue = queue;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets = 4;
    config.lru_purge_enable = true;
    rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
    // Start the httpd server
    httpd_uri_t auth = {
        .uri = "/auth",
        .method = HTTP_POST,
        .handler = auth_post_handler,
        .user_ctx = rest_context
    };

    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get_handler
    };

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &auth);
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
    }
}

void stop_ap_server(void)
{
    httpd_stop(server);
}


