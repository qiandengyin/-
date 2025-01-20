/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "sync_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define timer_read() sync_timer_read()
#define timer_read32() sync_timer_read32()
#define timer_elapsed(x) sync_timer_elapsed(x)
#define timer_elapsed32(x) sync_timer_elapsed32(x)

#ifdef __cplusplus
}
#endif
