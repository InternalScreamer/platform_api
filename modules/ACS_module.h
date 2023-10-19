
#ifndef ACS_MODULE_H_
#define ACS_MODULE_H_

#include "../../interfaces/power_mon_if.h"

// Esp Libraries
#include "esp_adc/adc_continuous.h"
#include "soc/soc_caps.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

// FreeRtos
#include "freertos/queue.h"
#include "driver/gptimer.h"

#define PWR_SAMPLE_SZ_BYTES 512
#define PWR_SAMPLE_SZ PWR_SAMPLE_SZ_BYTES/4
#define SAMPLE_ARR_SZ 120
#define OUT_ARR_SZ 60

typedef struct EspAcsStruct EspAcs_c;

struct EspAcsStruct {
    Pwr_mon_i baseClass;
    // Adc variables
    adc_channel_t channel;
    adc_continuous_handle_t handle;
    adc_continuous_config_t cali_handle;
    adc_unit_t unit;

    // Timer variables
    gptimer_handle_t timer;

    // RTOS Variables
    EventGroupHandle_t adc_evt;
    QueueHandle_t cont_queue;

    // Readings Index
    uint64_t one_idx;
    uint8_t samp_idx;
    uint8_t three_idx;
    uint8_t six_idx;
    uint8_t twelve_idx;
    uint8_t twenty_four_idx;
    uint32_t sample_freq;

    // Readings Arrays
    uint32_t sample_arr[SAMPLE_ARR_SZ];
    float one_hr[OUT_ARR_SZ];         // Mode 0
    float three_hr[OUT_ARR_SZ];       // Mode 1
    float six_hr[OUT_ARR_SZ];         // Mode 2
    float twelve_hr[OUT_ARR_SZ];      // Mode 3
    float twenty_four_hr[OUT_ARR_SZ]; // Mode 4
};

// This will have gptimer for frequent reads
extern const struct EspAcsClass {
    EspAcs_c (*new)(adc_channel_t* channel, adc_unit_t* channel_num, uint32_t sample_freq_hz, QueueHandle_t queue);
} EspAcs;

#endif