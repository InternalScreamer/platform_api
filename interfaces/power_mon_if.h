
#ifndef POWER_MON_IF_H_
#define POWER_MON_IF_H_

#include <stdint.h>

typedef struct PwrMonStruct Pwr_mon_i;

struct PwrMonStruct {
    void (*init)(Pwr_mon_i* interface);

    void (*stop)(Pwr_mon_i* interface);

    void (*start)(Pwr_mon_i* interface);

    void (*measure_power) (Pwr_mon_i* interface, uint32_t timeout_ms);

    void (*get_power_readings) (Pwr_mon_i* interface, float* data, uint16_t* num, uint8_t mode);

};

#endif
