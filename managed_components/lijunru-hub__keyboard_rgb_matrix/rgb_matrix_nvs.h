/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_log.h"

esp_err_t nvs_init_rgb_matrix();

esp_err_t nvs_flush_rgb_matrix(bool if_flush);

#ifdef __cplusplus
}
#endif
