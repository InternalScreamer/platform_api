
#include "controller.h"

// Freertos Libraries
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// Platform libraries
#include "relay.h"
#include "plug_server.h"
#include "ap_server.h"
#include "wifi_module.h"

// Standard Libs
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef enum controller_state {
    VALIDATE_NVS = 0,
    AP_SERVER,
    PLUG_SERVER,
} controller_state_t;

static QueueHandle_t s_queue;
controller_state_t s_state;
uint8_t s_retry_ap = 0;

static void controller_task(void* pvParams)
{
    uint8_t msg[MAX_MSG_SIZE];
    while(1) {
        if(xQueueReceive(s_queue, (void * )&msg, (TickType_t)portMAX_DELAY)) {
            uint8_t msg_id = msg[0];
            switch(s_state) {
                case VALIDATE_NVS: {
                // check the nvs wifi
                    if(wifi_load_credentials()) {
                        printf("NVS Valid\n");
                        wifi_init_sta();
                        wifi_start_sta(s_queue);
                        s_state = PLUG_SERVER;
                    } else {
                        wifi_init_ap();
                        wifi_start_ap(s_queue);
                        s_state = AP_SERVER;
                    }
                    break;
                }
                case AP_SERVER: {
                    if (msg_id == START_AP_SUCCESS) {
                        start_ap_server(s_queue);
                    } else if (msg_id == START_AP_FAILED) {
                        s_retry_ap++;
                        wifi_init_ap();
                        wifi_start_ap(s_queue);
                    } else if (msg_id == WIFI_DATA) {
                        char ssid[32];
                        char pwd[32];
                        memcpy(ssid, &msg[1], 32);
                        memcpy(pwd, &msg[33], 32);
                        stop_ap_server();
                        wifi_stop_ap();
                        wifi_save_credentials(ssid, pwd);
                        s_state = VALIDATE_NVS;
                    }
                    break;
                }
                case PLUG_SERVER: {
                    if(msg_id == START_STA_SUCCESS) {
                        start_plug_server(s_queue);
                    } else if (msg_id == START_STA_FAILED) {
                        wifi_init_ap();
                        wifi_start_ap(s_queue);
                        s_state = AP_SERVER;
                    } else if (msg_id == DELETE_DATA) {
                        stop_plug_server();
                        wifi_stop_sta();
                        wifi_delete_credentials();
                        s_state = VALIDATE_NVS;
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
}

void controller_init(void)
{
    printf("Controller Init\n");
    s_queue = xQueueCreate(12, MAX_MSG_SIZE);
    xTaskCreate(controller_task, "controller_task", 8196, NULL, 12, NULL);
    s_state = VALIDATE_NVS;
}

void controller_start(void)
{
    printf("Controller Start\n");
    uint8_t msg[32] = {0};
    msg[0] = START_NVS_VALIDATION;
    xQueueSend(s_queue, msg, 0);
}
