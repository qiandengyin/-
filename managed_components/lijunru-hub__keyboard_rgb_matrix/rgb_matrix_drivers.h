// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdint.h>

#include "led_strip.h"

typedef struct {
    /* Perform any initialisation required for the other driver functions to work. */
    void (*init)(void);
    /* Set the colour of a single LED in the buffer. */
    void (*set_color)(int index, uint8_t r, uint8_t g, uint8_t b);
    /* Set the colour of all LEDS on the keyboard in the buffer. */
    void (*set_color_all)(uint8_t r, uint8_t g, uint8_t b);
    /* Flush any buffered changes to the hardware. */
    void (*flush)(void);
} rgb_matrix_driver_t;

extern const rgb_matrix_driver_t rgb_matrix_driver;

void rgb_matrix_driver_init(led_strip_handle_t handle, uint32_t strip_num);
