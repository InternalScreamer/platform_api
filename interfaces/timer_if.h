
#ifndef TIMER_IF_H_
#define TIMER_IF_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct TimerStruct Timer_i;

typedef struct timer_config_s {
    uint8_t active; // whether the timer is running or not
    uint8_t hr; //0-23
    uint8_t min;// 0-59
    uint8_t sec; // 0-59
    uint8_t repeat; // 7 bit number (representing mon-sun)
    uint8_t action; // 0,1,2 (off, on, toggle)
} timer_config_t;

struct TimerStruct {

    void (*init)(Timer_i* interface);

    void (*set_timer)(Timer_i* interface, timer_config_t* config, uint8_t index);

    uint8_t (*get_num_timers)(Timer_i* interface);

    void (*get_timer)(Timer_i* interface, timer_config_t* cfg, uint8_t index);

    void (*delete_timer)(Timer_i* interface, uint8_t index);

    void (*set_active)(Timer_i* interface, uint8_t index, uint8_t active);

    void (*set_current_time)(Timer_i* interface, uint32_t utc_time);

    void (*set_update_method)(Timer_i* interface, bool manual);

    void (*deinit)(Timer_i* interface);

};

#endif //TIMER_IF_H_