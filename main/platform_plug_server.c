#include "platform_plug_server.h"
#include "esp_http_server.h"


#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"
#include "platform_relay.h"

static const char *TAG = "Plug Server";
extern const char plug_start[] asm("_binary_plug_control_html_start");
extern const char plug_end[] asm("_binary_plug_control_html_end");
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (5120)

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

static esp_err_t plug_page_get_handler(httpd_req_t *req)
{
    const uint32_t plug_len = plug_end - plug_start;
    ESP_LOGI(TAG, "Serve root");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, plug_start, plug_len);
    return ESP_OK;
}


static esp_err_t plug_ctrl_api_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
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
    int plug0 = cJSON_GetObjectItem(root, "plug0")->valueint;
    int plug1 = cJSON_GetObjectItem(root, "plug1")->valueint;
    ESP_LOGI(TAG, "plug0: %d", plug0);
    ESP_LOGI(TAG, "plug1: %d", plug1);
    set_relay(0, plug0);
    set_relay(1, plug1);
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post control value successfully");
    return ESP_OK;

    return ESP_OK;
}

static const httpd_uri_t plug_page_api_uri = {
    .uri = "/plug_control",
    .method = HTTP_GET,
    .handler = plug_page_get_handler
};

httpd_handle_t start_plug_server(void)
{

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets = 4;
    config.lru_purge_enable = true;
    rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &plug_page_api_uri);

        httpd_uri_t plgu_ctrl_api_uri = {
            .uri = "/api/v1/plug_ctrl",
            .method = HTTP_POST,
            .handler = plug_ctrl_api_post_handler,
            .user_ctx = rest_context
        };
        httpd_register_uri_handler(server, &plgu_ctrl_api_uri);
    }
    return server;
}
