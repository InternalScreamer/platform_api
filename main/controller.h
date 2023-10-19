#ifndef PLATFORM_CONTROLLER_H_

#define PLATFORM_CONTROLLER_H_

#define MAX_MSG_SIZE 72
#include <stdio.h>
// Msg IDs for controller

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Interfaces
#include "../interfaces/power_mon_if.h"

 enum controller_msg {
    START_NVS_VALIDATION = 0,
    START_STA_SUCCESS,
    START_STA_FAILED,
    START_AP_SUCCESS,
    START_AP_FAILED,
    WIFI_DATA,
    DELETE_DATA,
    TIMER_CONFIG,
    TIME_UPDATE,
    MEASURE_POWER,
};

// timer control msg

QueueHandle_t controller_init(void);

void controller_start(Pwr_mon_i* pwr_intf);

#endif //PLATFORM_CONTROLLER_H_
