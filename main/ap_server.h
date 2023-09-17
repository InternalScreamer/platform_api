
#ifndef AP_SERVER_H_
#define AP_SERVER_H_

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void start_ap_server(QueueHandle_t queue);

void stop_ap_server(void);

#endif // AP_SERVER_H_