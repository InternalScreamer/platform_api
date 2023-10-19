
#ifndef RELAY_IF_H_
#define RELAY_IF_H_

#include <stdint.h>

typedef struct RelayStruct Relay_i;

struct RelayStruct {
    void (*init)(Relay_i* self, uint32_t* relay_gpios, uint8_t gpio_num, uint8_t active_lvl);

    void (*set_relays)(Relay_i* self, uint16_t relay_bits, uint8_t num_relays);

    uint8_t (*get_num_relays)(Relay_i* self);

    void (*deinit)(Relay_i* self);
};
#endif
