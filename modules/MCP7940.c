
#include "MCP7940.h"

// Standard Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Interfaces
#include "../interfaces/timer_if.h"

// Freertos
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// ESP Libraries
#include "nvs_flash.h"
#include "nvs.h"

// helper libraries
#include "util.h"

#define FROM_IF(b) (McpRtc_c *)((char *)b - offsetof(McpRtc_c, baseClass))

static bool load_timers(McpRtc_c* self)
{
    size_t data_size = TIMER_CNT*6 + 2;
    uint8_t data_store[TIMER_CNT*6 + 2] = {0};
    nvs_open("storage", NVS_READWRITE, &self->mcp_nvs_handle);
    nvs_get_blob(self->mcp_nvs_handle, "timer_configs", data_store, &data_size);
    nvs_close(self->mcp_nvs_handle);
    uint16_t checksum = basic_checksum(data_store, TIMER_CNT*6);
    uint16_t verify_checksum = 0;
    memcpy(&verify_checksum, &data_store[TIMER_CNT*6], 2);
    printf("Checksum: %d\n", checksum);
    printf("Verify Checksum: %d\n", verify_checksum);
    if (verify_checksum != checksum) {
        return false;
    }
    memcpy(self->timer_cfgs, data_store, TIMER_CNT*6);
    return true;
}

static void save_timers(McpRtc_c* self)
{
    size_t data_size = TIMER_CNT*6 + 2;
    size_t fake_size = data_size;
    uint8_t data_store[TIMER_CNT*6 + 2] = {0};
    memcpy(&data_store[0], self->timer_cfgs, TIMER_CNT*6 + 2);
    uint16_t checksum = basic_checksum(data_store, TIMER_CNT*6);
    memcpy(&data_store[TIMER_CNT*6], &checksum, 2);
    nvs_open("storage", NVS_READWRITE, &self->mcp_nvs_handle);
    nvs_get_blob(self->mcp_nvs_handle, "timer_configs", NULL, &fake_size);
    nvs_set_blob(self->mcp_nvs_handle, "timer_configs", data_store, data_size);
    nvs_commit(self->mcp_nvs_handle);
    nvs_close(self->mcp_nvs_handle);
}

static void delete_timers(McpRtc_c* self)
{
    memset(self->timer_cfgs, 0xFF, TIMER_CNT*6 + 2);
    save_timers(self);
}

void mcp_set_timer(Timer_i* interface, timer_config_t* config, uint8_t index)
{
    if (interface == NULL || config == NULL || index > TIMER_CNT) {
        return;
    }
    McpRtc_c* self = FROM_IF(interface);
    self->timer_cfgs[index] = *config;
    printf("active: %d\n", self->timer_cfgs[index].active);
    printf("hr: %d\n", self->timer_cfgs[index].hr);
    printf("min: %d\n", self->timer_cfgs[index].min);
    save_timers(self);
    // Recalculate queue and alarm if timer is active
}

void mcp_get_timer(Timer_i* interface, timer_config_t* cfg, uint8_t index)
{
    if(interface == NULL || cfg == NULL || index > TIMER_CNT) {
        return;
    }
    McpRtc_c* self = FROM_IF(interface);
    *cfg = self->timer_cfgs[index];
}

static void mcp_init(Timer_i* interface)
{
    if (interface == NULL) {
        return;
    }
    McpRtc_c* self = FROM_IF(interface);
    if (load_timers(self) == false) {
        delete_timers(self);
        printf("No Valid timers\n");
        return;
    }

    for(uint8_t i = 0; i < TIMER_CNT; ++i) {
        printf("Active: %d\n", self->timer_cfgs[i].active);
        printf("Hour: %d\n", self->timer_cfgs[i].hr);
        printf("Min: %d\n", self->timer_cfgs[i].min);
        printf("Sec: %d\n", self->timer_cfgs[i].sec);
    }
    
    // Get current time
    // Start timer calculations
    // Get Queue ready 
    // Setup alarms
}

static McpRtc_c McpNew(void)
{
    McpRtc_c rtn = {
        .baseClass = {
            .init = &mcp_init,
            .get_timer = &mcp_get_timer,
            .set_timer = &mcp_set_timer,
        },
    };

    // Setup the I2c bus
    return rtn;
}

const struct McpRtcClass McpRtc = { .new = &McpNew };

#undef FROM_IF