#ifndef PLATFORM_CONTROLLER_H_

#define PLATFORM_CONTROLLER_H_

#define MAX_MSG_SIZE 72
#include <stdio.h>
// Msg IDs for controller
 enum controller_msg {
    START_NVS_VALIDATION = 0,
    START_STA_SUCCESS,
    START_STA_FAILED,
    START_AP_SUCCESS,
    START_AP_FAILED,
    WIFI_DATA,
    DELETE_DATA,
};

void controller_init(void);

void controller_start(void);

#endif //PLATFORM_CONTROLLER_H_
