#include "plug_server.h"
#include "esp_http_server.h"


#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Platform Libraries
#include "relay.h"
#include "controller.h"


// Interfaces
#include "../interfaces/power_mon_if.h"

static const char *TAG = "Plug Server";
extern const char dashboard_start[] asm("_binary_dashboard_html_start");
extern const char dashboard_end[] asm("_binary_dashboard_html_end");

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (5120)

static httpd_handle_t server = NULL;
static QueueHandle_t s_queue;
static Pwr_mon_i* s_pwr_mon;
static uint8_t pwr_mon_mode = 0;

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

rest_server_context_t rest_context;

// Helper function
static void rec_http_buff_data(httpd_req_t *req, char *buf)
{
    int total_len = req->content_len;
    int cur_len = 0;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';
}

static esp_err_t webpage_handler(httpd_req_t *req)
{
    const uint32_t dash_len = dashboard_end - dashboard_start;
    ESP_LOGI(TAG, "Serve root");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, dashboard_start, dash_len);
    return ESP_OK;
}

static esp_err_t relay_api_post_handler(httpd_req_t *req)
{
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    rec_http_buff_data(req, buf);
    cJSON *root = cJSON_Parse(buf);
    int plug0 = cJSON_GetObjectItem(root, "sw0")->valueint;
    int plug1 = cJSON_GetObjectItem(root, "sw1")->valueint;
    set_relay(0, plug0);
    set_relay(1, plug1);
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post control value successfully");
    return ESP_OK;
}

static esp_err_t relay_api_get_handler(httpd_req_t *req)
{
    uint8_t sw0 = get_relay(0);
    uint8_t sw1 = get_relay(1);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "sw0", sw0);
    cJSON_AddNumberToObject(root, "sw1", sw1);
    const char *relay_req = cJSON_Print(root);
    httpd_resp_sendstr(req, relay_req);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t pwr_mon_api_get_handler(httpd_req_t *req)
{
    float data[60] = {0.0};
    uint16_t num = 0;
    s_pwr_mon->get_power_readings(s_pwr_mon, data, &num, pwr_mon_mode);
    cJSON *root = cJSON_CreateObject();
    cJSON *pwr_readings = cJSON_CreateFloatArray(data, 60);
    cJSON_AddItemToObject(root, "power_data", pwr_readings);
    cJSON_AddNumberToObject(root, "mode", pwr_mon_mode);
    const char *pwr_read_req = cJSON_Print(root);
    httpd_resp_sendstr(req, pwr_read_req);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t pwr_mon_api_post_handler(httpd_req_t *req)
{
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    rec_http_buff_data(req, buf);
    cJSON *root = cJSON_Parse(buf);
    pwr_mon_mode = cJSON_GetObjectItem(root, "mode")->valueint;
    return ESP_OK;
}

/*
static esp_err_t reset_api_post_handler(httpd_req_t *req)
{
}
*/
// Services the dashboard
static const httpd_uri_t webpage = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = webpage_handler,
};

// API Endpoints
httpd_uri_t relay_post_endpoint = {
    // POST command to set relay state 
    .uri = "/api/v1/relays",
    .method = HTTP_POST,
    .handler = relay_api_post_handler,
    .user_ctx = &rest_context
};

httpd_uri_t relay_get_endpoint = {
    // GET command for the relay state
    .uri = "/api/v1/relays",
    .method = HTTP_GET,
    .handler = relay_api_get_handler,
    .user_ctx = &rest_context
};

httpd_uri_t power_monitor_get_endpoint = {
    // GET command for power monitor
    .uri = "/api/v1/power_monitor",
    .method = HTTP_GET,
    .handler = pwr_mon_api_get_handler,
    .user_ctx = &rest_context
};

httpd_uri_t power_monitor_post_endpoint = {
    // GET command for power monitor
    .uri = "/api/v1/power_monitor",
    .method = HTTP_POST,
    .handler = pwr_mon_api_post_handler,
    .user_ctx = &rest_context
};
/*
httpd_uri_t delete_data_post_endpoint = {
    // PUT command to delete data and reset 
    .uri = "/api/v1/reset_device",
    .method = HTTP_GET,
    .handler = delete_api_post_handler,
    .user_ctx = &rest_context
};
*/
// TODO: Timer GET and POST

void start_plug_server(QueueHandle_t queue, Pwr_mon_i* pwr_intf)
{
    s_queue = queue;
    s_pwr_mon = pwr_intf;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets = 4;
    config.lru_purge_enable = true;
    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");

        // Webpage being served
        httpd_register_uri_handler(server, &webpage);

        // API Endpoints
        httpd_register_uri_handler(server, &relay_post_endpoint);
        httpd_register_uri_handler(server, &relay_get_endpoint);
        httpd_register_uri_handler(server, &power_monitor_get_endpoint);
        httpd_register_uri_handler(server, &power_monitor_post_endpoint);
    }
}

void stop_plug_server(void)
{
    httpd_stop(server);
}
