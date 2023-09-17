
#include "wifi_module.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "nvs_flash.h"
#include "nvs.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/inet.h"

//platform libraries
#include "controller.h"

//standard libraries
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// helper libraries
#include "util.h"

#define CONNECT_ATTEMPTS 5

static const char *TAG = "wifi module";

static struct WifiStruct {
    // All necessary class variables
    char ssid[32];
    char password[32];
    uint8_t security_type;
    uint8_t retry_num;
    bool valid_nvs_data;
    bool connected;
    esp_netif_t* wifi_if;
    nvs_handle_t wifi_nvs_handle;
} s_self = { .ssid = "",
             .password = "",
             .security_type = WIFI_AUTH_WPA2_PSK,
             .retry_num = 0,
             .valid_nvs_data = false,
             .connected = false,
};

//Wifi Variables
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static QueueHandle_t s_queue;

// Wifi Handler
static void wifi_sta_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < CONNECT_ATTEMPTS) {
            esp_wifi_connect();
            s_self.retry_num++;
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        s_self.retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_ap_event_handler(void *arg, esp_event_base_t event_base, 
                                int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void wifi_save_credentials(char* ssid, char* password)
{
    uint8_t data_store[MAX_SSID_LEN + MAX_PASSWORD_LEN +2];
    size_t index = 0;
    memcpy(&data_store[index], ssid, MAX_SSID_LEN);
    index += MAX_SSID_LEN;
    memcpy(&data_store[index], password, MAX_PASSWORD_LEN);
    index += MAX_PASSWORD_LEN;
    uint16_t checksum = basic_checksum(data_store, MAX_SSID_LEN + MAX_PASSWORD_LEN);
    memcpy(&data_store[index], &checksum, 2);
    index += 2;
    nvs_open("storage", NVS_READWRITE, &s_self.wifi_nvs_handle);
    size_t fake_size = index;
    nvs_get_blob(s_self.wifi_nvs_handle, "wifi_creds", NULL, &fake_size);
    nvs_set_blob(s_self.wifi_nvs_handle, "wifi_creds", data_store, index);
    nvs_commit(s_self.wifi_nvs_handle);
    nvs_close(s_self.wifi_nvs_handle);
    uint8_t msg[3] = {0};
    msg[0] = START_NVS_VALIDATION;
    xQueueSend(s_queue, msg, 0);
}

bool wifi_load_credentials(void)
{
    size_t data_size = MAX_SSID_LEN + MAX_PASSWORD_LEN +2;
    uint8_t data_store[MAX_SSID_LEN + MAX_PASSWORD_LEN +2];
    nvs_open("storage", NVS_READWRITE, &s_self.wifi_nvs_handle);
    nvs_get_blob(s_self.wifi_nvs_handle, "wifi_creds", data_store, &data_size);
    nvs_close(s_self.wifi_nvs_handle);
    size_t index = 0;
    uint16_t verify_checksum = 0;
    memcpy(s_self.ssid, &data_store[index], MAX_SSID_LEN);
    index += MAX_SSID_LEN;
    memcpy(s_self.password, &data_store[index], MAX_PASSWORD_LEN);
    index += MAX_PASSWORD_LEN;
    memcpy(&verify_checksum, &data_store[index], 2);
    uint16_t checksum = basic_checksum(data_store, index);
    if (checksum != verify_checksum) {
        s_self.valid_nvs_data = false;
    } else {
        s_self.valid_nvs_data = true;
    }
    return s_self.valid_nvs_data;
}

void wifi_delete_credentials(void)
{
    uint8_t data_store[MAX_SSID_LEN + MAX_PASSWORD_LEN +2];
    memset(data_store, 0xFF, MAX_SSID_LEN + MAX_PASSWORD_LEN +2);
    nvs_open("storage", NVS_READWRITE, &s_self.wifi_nvs_handle);
    size_t data_size = MAX_SSID_LEN + MAX_PASSWORD_LEN +2;
    nvs_get_blob(s_self.wifi_nvs_handle, "wifi_creds", NULL, &data_size);
    data_size = MAX_SSID_LEN + MAX_PASSWORD_LEN +2;
    nvs_set_blob(s_self.wifi_nvs_handle, "wifi_creds", data_store, data_size);
    nvs_commit(s_self.wifi_nvs_handle);
    nvs_close(s_self.wifi_nvs_handle);
    uint8_t msg[3] = {0};
    msg[0] = START_NVS_VALIDATION;
    xQueueSend(s_queue, msg, 0);
}

void wifi_init_sta(void)
{
    //Configuring wifi task
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    s_self.wifi_if = esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_netif_set_hostname(s_self.wifi_if, "platform_api");
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_sta_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_sta_event_handler,
                                                        NULL,
                                                        &instance_got_ip));
}

void wifi_init_ap(void)
{
    const char *hostname;
    esp_netif_init();
    esp_event_loop_create_default();
    s_self.wifi_if = esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_netif_set_hostname(s_self.wifi_if, "platform_api");
    esp_wifi_init(&cfg);
    esp_netif_get_hostname(s_self.wifi_if, &hostname);
    printf("Hostname: %s\n", hostname);
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_ap_event_handler, NULL);
}

void wifi_start_sta(QueueHandle_t queue)
{
    if (s_self.valid_nvs_data == false) {
        s_self.connected = false;
        return;
    }
    s_queue = queue;
    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = s_self.security_type,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    memcpy(wifi_cfg.sta.ssid, s_self.ssid, 32);
    memcpy(wifi_cfg.sta.password, s_self.password, 32);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
        uint8_t msg[3]= {0};
    if (bits & WIFI_CONNECTED_BIT) {
        //Successful wifi Connect
        msg[0] = START_STA_SUCCESS;
        s_self.connected = true;
    } else if (bits & WIFI_FAIL_BIT) {
        s_self.connected = false;
        msg[0] = START_STA_FAILED;
    }
    xQueueSend(s_queue, msg, 0);
}

void wifi_start_ap(QueueHandle_t queue)
{
    s_queue = queue;
    wifi_config_t wifi_cfg = {
        .ap = {
            .ssid = "platform_api",
            .ssid_len = strlen("platform_api"),
            .password = "",
            .max_connection = 2,
            .authmode = WIFI_AUTH_OPEN
        },
    };
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_cfg);
    esp_wifi_start();
    uint8_t msg = START_AP_SUCCESS;
    xQueueSend(s_queue, &msg, 0);
}

void wifi_stop_sta(void)
{
    esp_wifi_deinit();
}

void wifi_stop_ap(void)
{
    esp_wifi_deinit();
}
