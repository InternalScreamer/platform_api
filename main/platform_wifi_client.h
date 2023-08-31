#ifndef PLATFORM_WIFI_CLIENT
#define PLATFORM_WIFI_CLIENT
#include <stdint.h>

typedef struct platform_wifi_cfg_s {
    uint8_t ssid[32];
    uint8_t password[32];
    uint8_t sec_type;
    uint8_t conn_attempts;
} platform_wifi_cfg_t;

void wifi_client_init(void);

void wifi_client_configure(platform_wifi_cfg_t* config);

void wifi_client_connect(void);

void wifi_client_disconnect(void);

#endif // PLATFORM_WIFI_CLIENT