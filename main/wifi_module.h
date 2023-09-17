#ifndef WIFI_MODULE_H_
#define WIFI_MODULE_H_

#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define MAX_SSID_LEN 32
#define MAX_PASSWORD_LEN 32

void wifi_init_sta(void);

void wifi_init_ap(void);

void wifi_start_sta(QueueHandle_t queue);

void wifi_start_ap(QueueHandle_t queue);

bool wifi_is_ap(void);

bool wifi_is_sta(void);

void wifi_stop_ap(void);

void wifi_stop_sta(void);

void wifi_save_credentials(char* ssid, char* password);

bool wifi_load_credentials(void);

void wifi_delete_credentials(void);

#endif //WIFI_MODULE_H_
