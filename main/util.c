
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