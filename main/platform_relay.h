
#ifndef PLATFORM_RELAY_H
#define PLATFORM_RELAY_H

#include <stdint.h>

#define RELAY_CHANNELS 2

typedef struct platform_relay_s{
    uint32_t pin_nums[RELAY_CHANNELS];
    uint8_t active_lvl[RELAY_CHANNELS];
    uint8_t pwr_on[RELAY_CHANNELS];
    uint8_t relay_num;
} platform_relay_t;

void init_relay(uint32_t* gpio,uint8_t* active_level, uint8_t gpio_num);

uint8_t get_relay_num(void);

uint8_t get_relay(uint8_t relay_num);

void set_relay(uint32_t relay_num, uint8_t pwr_on);

#endif