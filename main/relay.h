
#ifndef RELAY_H_
#define RELAY_H_

#include <stdint.h>

#define RELAY_CHANNELS 2

typedef struct platform_relay_s{
    uint32_t pin_nums[RELAY_CHANNELS];
    uint8_t active_lvl[RELAY_CHANNELS];
    uint8_t pwr_on[RELAY_CHANNELS];
    uint8_t relay_num;
} platform_relay_t;

void relay_init(uint32_t* gpio,uint8_t* active_level, uint8_t gpio_num);

uint8_t get_relay_num(void);

uint8_t get_relay(uint8_t relay_num);

void set_relay(uint32_t relay_num, uint8_t pwr_on);

void relay_close(void);

void set_relay_timers(uint8_t timer_num, uint16_t sample_freq);

#endif // RELAY_H_
