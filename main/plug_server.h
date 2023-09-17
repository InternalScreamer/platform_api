
#ifndef PLUG_SERVER_H_
#define PLUG_SERVER_H_

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
// Plug Server Object

void start_plug_server(QueueHandle_t queue);

void stop_plug_server(void);


#endif //PLUG_SERVER_H_