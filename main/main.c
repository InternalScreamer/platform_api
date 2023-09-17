
#include <sys/param.h>
#include <strings.h>
#include "unistd.h"
#include <stdint.h>
#include "esp_system.h"

#include "nvs_flash.h"
#include "nvs.h"
#include "relay.h"

// platform specific libraries
#include "controller.h"

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    // Relay Initialization
   uint32_t gpios[2] = {25, 21};
   uint8_t gpio_num = 2;
   uint8_t active_level[2] = {0, 0};
   relay_init(gpios, active_level, gpio_num);

    controller_init();
    controller_start();
    while(1) {
        sleep(5);
    }
}
