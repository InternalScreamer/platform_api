
#ifndef PLUG_SERVER_H_
#define PLUG_SERVER_H_

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Interfaces
#include "../interfaces/power_mon_if.h"

// Plug Server Object

void start_plug_server(QueueHandle_t queue, Pwr_mon_i* pwr_intf);

void stop_plug_server(void);

#endif //PLUG_SERVER_H_