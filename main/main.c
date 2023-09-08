
#include <sys/param.h>
#include <strings.h>
#include "unistd.h"
#include <stdint.h>

#include "esp_http_server.h"
#include "nvs_flash.h"

// platform specific libraries
//#include "platform_ap_server.h"
#include "platform_plug_server.h"
#include "platform_wifi_client.h"
#include "platform_relay.h"

void app_main(void)
{
    // Relay Initialization
   uint32_t gpios[2] = {18, 17};
   uint8_t gpio_num = 2;
   uint8_t active_level[2] = {0, 0};
   init_relay(gpios, active_level, gpio_num);

    // Wifi Initialization
    platform_wifi_cfg_t wifi_cfg = {
    .ssid = "SpectrumSetup-58",
    .password = "fastcrown231",
    .conn_attempts = 5,
   };

    // Initialize NVS needed by Wi-Fi
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_client_init();
    wifi_client_configure(&wifi_cfg);
    wifi_client_connect();

    // Start the server for the first time
    static httpd_handle_t server = NULL;
    server = start_plug_server();
    // Start the DNS server that will redirect all queries to the softAP IP

    while(server) {
        sleep(5);
    }
}
