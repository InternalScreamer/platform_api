
#include "util.h"
#include <stdint.h>
#include <stddef.h>

uint16_t basic_checksum(uint8_t* arr, size_t arr_size)
{
    uint16_t checksum = 0;
    for (size_t i = 0; i < arr_size; ++i) {
        checksum += arr[i];
    }
    return checksum;
}

// float average
float arr_float_avg(float* arr, uint16_t arr_size)
{
    float avg = 0.0;
    for(uint16_t i = 0; i < arr_size; ++i) {
        avg += arr[i];
    }
    return (float)(avg / arr_size);
}

// standard deviation
float arr_u32_avg(uint32_t* arr, uint16_t arr_size)
{
    uint32_t avg = 0.0;
    for(uint16_t i = 0; i < arr_size; ++i) {
        avg += arr[i];
    }
    return (float)(avg / arr_size);
}

