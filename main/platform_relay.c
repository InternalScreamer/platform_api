#include "platform_relay.h"
#include <stdint.h>
#include <string.h>
#include "driver/gpio.h"

platform_relay_t s_relay;

void init_relay(uint32_t* gpio, uint8_t* active_level, uint8_t gpio_num)
{
    if (gpio == NULL || active_level == NULL || gpio_num == 0) {
        return;
    }
    // Generating Pin Bit Mask
    //zero-initialize the config structure.
    //gpio_drive_cap_t drv_gpio = GPIO_DRIVE_CAP_MAX;
    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL << gpio[0]) | (1ULL << gpio[1]);
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT_OD;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 1;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    memset(s_relay.pwr_on,0, gpio_num);
    memcpy(s_relay.active_lvl, active_level, gpio_num);
    s_relay.pin_nums[0] = gpio[0];
    s_relay.pin_nums[1] = gpio[1];
    s_relay.relay_num = gpio_num;
    set_relay(0, 1);
    set_relay(1, 1);
}

uint8_t get_relay_num(void)
{

    return s_relay.relay_num;
}

uint8_t get_relay(uint8_t relay_num)
{
    return 0;
}

void set_relay(uint32_t relay_num, uint8_t pwr_on)
{
    if(relay_num >= s_relay.relay_num) {
        return;
    }
    uint8_t gpio_lvl = 0;
    if (pwr_on) {
        if (s_relay.active_lvl[relay_num]) {
            gpio_lvl = 1;
        }
    } else {
        if (s_relay.active_lvl[relay_num] == 0) {
            gpio_lvl = 1;
        }
    }
    printf("gpio_level: %d\n", gpio_lvl);
    printf("Pin Number: %ld\n", s_relay.pin_nums[relay_num]);
    gpio_set_level(s_relay.pin_nums[relay_num], gpio_lvl);
}