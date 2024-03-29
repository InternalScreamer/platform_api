
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

// Interfaces
#include "../interfaces/power_mon_if.h"

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
static Pwr_mon_i* s_pwr_mon;
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
                        printf("NVS Failed");
                        wifi_init_ap();
                        wifi_start_ap(s_queue);
                        s_state = AP_SERVER;
                    }
                    break;
                }
                case AP_SERVER: {
                    if (msg_id == START_AP_SUCCESS) {
                        printf("AP Success");
                        start_ap_server(s_queue);
                    } else if (msg_id == START_AP_FAILED) {
                        printf("AP failed");
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
                // This is where all the main functionality will be
                case PLUG_SERVER: {
                    if(msg_id == START_STA_SUCCESS) {
                        printf("STA SUCCESS");
                        start_plug_server(s_queue, s_pwr_mon);
                        s_pwr_mon->init(s_pwr_mon);
                        s_pwr_mon->start(s_pwr_mon);
                    } else if (msg_id == START_STA_FAILED) {
                        printf("STA FAILED");
                        wifi_stop_sta();
                        wifi_delete_credentials();
                        s_state = VALIDATE_NVS;
                    } else if (msg_id == DELETE_DATA) {
                        // Data Delete command
                        s_pwr_mon->stop(s_pwr_mon);
                        stop_plug_server();
                        wifi_stop_sta();
                        wifi_delete_credentials();
                        s_state = VALIDATE_NVS;
                    } else if (msg_id == MEASURE_POWER) {
                        // Periodic Power Measurement
                        s_pwr_mon->measure_power(s_pwr_mon, 100);
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
}

QueueHandle_t controller_init(void)
{
    printf("Controller Init\n");
    s_queue = xQueueCreate(12, MAX_MSG_SIZE);
    xTaskCreate(controller_task, "controller_task", 16384, NULL, 12, NULL);
    s_state = VALIDATE_NVS;
    return s_queue;
}

void controller_start(Pwr_mon_i* pwr_intf)
{
    printf("Controller Start\n");
    uint8_t msg[32] = {0};
    msg[0] = START_NVS_VALIDATION;
    s_pwr_mon = pwr_intf;
    xQueueSend(s_queue, msg, 0);
}
