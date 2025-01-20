/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_timer.h"

uint16_t sync_timer_read(void)
{
    int64_t time = esp_timer_get_time() / 1000;
    // return low 16 bits
    return (time & 0xFFFF);
}

uint32_t sync_timer_read32(void)
{
    int64_t time = esp_timer_get_time() / 1000;
    // return low 32 bits
    return (time & 0xFFFFFFFF);
}

uint16_t sync_timer_elapsed(uint16_t last)
{
    int64_t time = esp_timer_get_time() / 1000;
    // return low 16 bits
    return (time - last) & 0xFFFF;
}

uint32_t sync_timer_elapsed32(uint32_t last)
{
    int64_t time = esp_timer_get_time() / 1000;
    // return low 32 bits
    return (time - last) & 0xFFFFFFFF;
}
