
#ifndef MCP7940_H_
#define MCP7940_H_

#include "../interfaces/timer_if.h"

#include <stdint.h>
#include "nvs.h"

#define TIMER_CNT 8

typedef struct Mcp7940Struct McpRtc_c;

struct Mcp7940Struct {
    Timer_i baseClass;
    timer_config_t timer_cfgs[TIMER_CNT];
    uint32_t utc_time;
    nvs_handle_t mcp_nvs_handle;
};

extern const struct McpRtcClass {
    McpRtc_c (*new)(void);
} McpRtc;

#endif //MCP7940_H_
