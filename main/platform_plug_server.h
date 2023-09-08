
#ifndef PLATFORM_PLUG_SERVER_H
#define PLATFORM_PLUG_SERVER_H

#include "esp_http_server.h"

// Plug Server Object

httpd_handle_t start_plug_server(void);

void stop_plug_server(void);

#endif //PLATFORM_PLUG_SERVER_H