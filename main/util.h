
#ifndef UTIL_H_
#define UTIL_H_

#include <stdint.h>
#include <stddef.h>

uint16_t basic_checksum(uint8_t* arr, size_t arr_size);

float arr_float_avg(float* arr, uint16_t arr_size);

float arr_u32_avg(uint32_t* arr, uint16_t arr_size);


#endif // UTIL_H_