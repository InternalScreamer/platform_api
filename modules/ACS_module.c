
#include "ACS_module.h"
#include "util.h"
#include "controller.h"

// Standard Libraries
#include <math.h>
#include <stdio.h>
#include <string.h>

//Interfaces
#include "../../interfaces/power_mon_if.h"

//ESP Libraries
#include "esp_adc/adc_continuous.h"
#include "soc/soc_caps.h"

//Freertos
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

//Drivers
#include "driver/gptimer.h"

#define CONV_DONE BIT0

#define MODE_1_IND_SZ 3
#define MODE_2_IND_SZ 6
#define MODE_3_IND_SZ 12
#define MODE_4_IND_SZ 24

#define FROM_IF(b) (EspAcs_c *)((char *)b - offsetof(EspAcs_c, baseClass))

static bool IRAM_ATTR timer_alarm_cb(gptimer_handle_t timer, 
                                     const gptimer_alarm_event_data_t *edata, 
                                     void *user_data)
{
    BaseType_t high_task_awoken = pdFALSE;
    QueueHandle_t queue = (QueueHandle_t)user_data;
    // Retrieve count value and send to queue
    uint8_t msg[1];
    msg[0] = MEASURE_POWER;
    xQueueSendFromISR(queue, msg, &high_task_awoken);
    // return whether we need to yield at the end of ISR
    return (high_task_awoken == pdTRUE);
}

static bool IRAM_ATTR conv_done_cb(adc_continuous_handle_t handle, 
                                  const adc_continuous_evt_data_t *edata, 
                                  void *user_data)
{
    EspAcs_c* self = (EspAcs_c*) user_data;
    xEventGroupSetBits(self->adc_evt, BIT0);
    return true;
}

static void esp_acs_init(Pwr_mon_i* interface)
{
    if (interface == NULL) {
        return;
    }
    EspAcs_c* self = FROM_IF(interface);

    // Adc Initialization
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size = PWR_SAMPLE_SZ_BYTES,
    };
    adc_continuous_new_handle(&adc_config, &self->handle);
    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = self->sample_freq,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    };
    adc_digi_pattern_config_t adc_pattern;
    adc_pattern.atten = ADC_ATTEN_DB_11;
    adc_pattern.channel = self->channel;
    adc_pattern.unit = self->unit;
    adc_pattern.bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;
    dig_cfg.pattern_num = 1;
    dig_cfg.adc_pattern = &adc_pattern;
    adc_continuous_config(self->handle, &dig_cfg);

    // Timer Initialization
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1MHz, 1 tick=1us
    };
    gptimer_new_timer(&timer_config, &self->timer);

    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_alarm_cb,
    };
    gptimer_register_event_callbacks(self->timer, &cbs, self->cont_queue);
}

static void add_to_readings(EspAcs_c* self) {
    // Average the sample
    self->one_hr[self->one_idx % OUT_ARR_SZ] = arr_u32_avg(self->sample_arr, SAMPLE_ARR_SZ);
    printf("Hour_Array %lld: %f\n", self->one_idx % OUT_ARR_SZ, self->one_hr[self->one_idx % OUT_ARR_SZ]);
    self->one_idx++;
    uint8_t read_idx = 0;
    if (self->one_idx % MODE_1_IND_SZ == 0) {
        // average three hour
        read_idx = (MODE_1_IND_SZ * self->three_idx) % OUT_ARR_SZ;
        self->three_hr[self->three_idx] = arr_float_avg(&self->one_hr[read_idx], MODE_1_IND_SZ);
        printf("3 Hour Array %d: %f\n", self->three_idx % OUT_ARR_SZ, self->three_hr[self->three_idx % OUT_ARR_SZ]);
        self->three_idx = (self->three_idx + 1) % OUT_ARR_SZ;
    }
    if (self->one_idx % MODE_2_IND_SZ == 0) {
        // average six hour
        read_idx = (MODE_2_IND_SZ * self->six_idx) % OUT_ARR_SZ;
        self->six_hr[self->six_idx] = arr_float_avg(&self->one_hr[read_idx], MODE_2_IND_SZ);
        printf("6 Hour Array %d: %f\n", self->six_idx % OUT_ARR_SZ, self->six_hr[self->six_idx % OUT_ARR_SZ]);
        self->six_idx = (self->six_idx + 1) % OUT_ARR_SZ;
    }
    if (self->one_idx % MODE_3_IND_SZ == 0) {
        // average twelve hour
        read_idx = (MODE_3_IND_SZ * self->twelve_idx) % OUT_ARR_SZ;
        self->twelve_hr[self->twelve_idx] = arr_float_avg(&self->one_hr[read_idx], MODE_3_IND_SZ);
        printf("12 Hour Array %d: %f\n", self->twelve_idx % OUT_ARR_SZ, self->twelve_hr[self->twelve_idx % OUT_ARR_SZ]);
        self->twelve_idx = (self->twelve_idx + 1) % OUT_ARR_SZ;
    }
    if (self->one_idx % MODE_4_IND_SZ == 0) {
        // average twenty four hour
        read_idx = (MODE_4_IND_SZ * self->twenty_four_idx) % OUT_ARR_SZ;
        self->twenty_four_hr[self->twelve_idx] = arr_float_avg(&self->one_hr[read_idx], MODE_4_IND_SZ);
        printf("24 Hour Array %d: %f\n", self->twenty_four_idx % OUT_ARR_SZ, self->twenty_four_hr[self->twenty_four_idx % OUT_ARR_SZ]);
        self->twenty_four_idx = (self->twenty_four_idx + 1) % OUT_ARR_SZ;
    }
}

static void esp_acs_measure_power(Pwr_mon_i* interface, uint32_t timeout_ms)
{
    if(interface == NULL) {
        return;
    }
    EspAcs_c* self = FROM_IF(interface);
    uint32_t ret_num = 0;
    uint8_t result[PWR_SAMPLE_SZ_BYTES] = {0};
    adc_continuous_read(self->handle, result, PWR_SAMPLE_SZ_BYTES, &ret_num, timeout_ms);
    TickType_t timeoutTicks = portTICK_PERIOD_MS * timeout_ms;
    EventBits_t bits = xEventGroupWaitBits(self->adc_evt, BIT0, pdFALSE, pdFALSE, timeoutTicks);
    if (bits & BIT0) {
        uint32_t total = 0;
        for (uint16_t i = 0; i < PWR_SAMPLE_SZ_BYTES; i+= 4) {
            adc_digi_output_data_t* p  =(void*)&result[i];
            total += abs(p->type2.data - 2047);
        }
        self->sample_arr[self->samp_idx] = (uint32_t)(total/128);
        self->samp_idx++;;
        if (self->samp_idx == SAMPLE_ARR_SZ) {
            self->samp_idx = 0;
            add_to_readings(self);
        }
    }
}

static void esp_acs_stop(Pwr_mon_i* interface)
{
    if(interface == NULL) {
        return;
    }
    EspAcs_c* self = FROM_IF(interface);
    adc_continuous_stop(self->handle);
    adc_continuous_deinit(self->handle);
    gptimer_stop(self->timer);
    gptimer_del_timer(self->timer);
}

static void esp_acs_start(Pwr_mon_i* interface)
{
    if(interface == NULL) {
        return;
    }
    EspAcs_c* self = FROM_IF(interface);

    // Start the ADC
    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = conv_done_cb,
    };
    adc_continuous_register_event_callbacks(self->handle, &cbs, self);
    adc_continuous_start(self->handle);
    // Start the timer
    gptimer_enable(self->timer);
    gptimer_alarm_config_t alarm_config1 = {
        .alarm_count = 50000, // period = 1s
        .flags.auto_reload_on_alarm = true,
    };
    gptimer_set_alarm_action(self->timer, &alarm_config1);
    gptimer_start(self->timer);
}

static void esp_acs_get_pow_readings(Pwr_mon_i* interface, float* data, uint16_t* num_readings, uint8_t mode)
{
    if(interface == NULL) {
        return;
    }
    float* arr_copy = NULL;
    EspAcs_c* self = FROM_IF(interface);
    *num_readings = OUT_ARR_SZ;
    if(mode == 0) {
        arr_copy = self->one_hr;
    } else if (mode == 1) {
        arr_copy = self->three_hr;
    } else if (mode == 2) {
        arr_copy = self->six_hr;
    } else if (mode == 3) {
        arr_copy = self->twelve_hr;
    } else if (mode == 4) {
        arr_copy = self->twenty_four_hr;
    }
    if (arr_copy != NULL) {
        for (uint8_t i = 0; i < OUT_ARR_SZ; i++) {
            data[i] = arr_copy[i] * 0.333; // Calculated adc to power conversion
        }
    }

}

static EspAcs_c EspAcsNew(adc_channel_t* channel, adc_unit_t* unit, uint32_t sample_freq_hz, QueueHandle_t queue)
{
    EspAcs_c ret = {
        .baseClass = {
            .init = &esp_acs_init,
            .stop = esp_acs_stop,
            .start = esp_acs_start,
            .measure_power = esp_acs_measure_power,
            .get_power_readings = esp_acs_get_pow_readings,
        },
    };
    if(channel) {
        ret.channel = *channel;
    }
    if(unit) {
        ret.unit = *unit;
    }
    if(sample_freq_hz > 0) {
        ret.sample_freq = sample_freq_hz;
    }
    if (queue) {
        ret.cont_queue = queue;
    }
    ret.adc_evt = xEventGroupCreate();
    return ret;
}

const struct EspAcsClass EspAcs = { .new = &EspAcsNew };

#undef FROM_IF