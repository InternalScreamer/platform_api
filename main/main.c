
#include <sys/param.h>
#include <strings.h>
#include "unistd.h"
#include <stdint.h>
#include "esp_system.h"

#include "nvs_flash.h"
#include "nvs.h"

#include "relay.h"
#include "hal/adc_types.h"
#include "../interfaces/power_mon_if.h"
#include "../interfaces/timer_if.h"

// Modules
#include "../modules/ACS_module.h"
//#include "../modules/MCP7940.h"

//freertos
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// Power monitor
Pwr_mon_i* s_pwr_mon;
EspAcs_c acs_pwr_mon;

// RTC Timer
//Timer_i* s_timer;
//McpRtc_c mcp_timer;

// Controller
QueueHandle_t controller_queue;


// platform specific libraries
#include "controller.h"

void power_monitor_init(QueueHandle_t queue)
{
    adc_channel_t channel = ADC_CHANNEL_9;
    adc_unit_t unit  = ADC_UNIT_1;
    uint32_t samp_freq = 10000;
    acs_pwr_mon = EspAcs.new(&channel, &unit, samp_freq, queue);
    s_pwr_mon = &acs_pwr_mon.baseClass;
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    // Relay Initialization
    /*
   uint32_t gpios[2] = {3, 9};
   uint8_t gpio_num = 2;
   uint8_t active_level[2] = {1, 1};
   relay_init(gpios, active_level, gpio_num);

    controller_init();
    controller_start();
   
   */
  /*
   uint32_t gpios[2] = {3, 9};
   uint8_t gpio_num = 2;
   uint8_t active_level[2] = {1, 1};
   relay_init(gpios, active_level, gpio_num);
   */
    uint32_t gpios[2] = {3, 9};
    uint8_t gpio_num = 2;
    uint8_t active_level[2] = {1, 1};
    relay_init(gpios, active_level, gpio_num);
    controller_queue = controller_init();
    power_monitor_init(controller_queue);
    controller_start(s_pwr_mon);
}
