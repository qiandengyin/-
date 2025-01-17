/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

enum
{
    MODE_HID_USB = 0,
    MODE_HID_BLE,
    MODE_HID_ESPNOW,
    MODE_HID_UDP,
    MODE_HID_MAX,
};

enum
{
    LED_MODE_BREATH = 0,
    LED_MODE_FLASH,
};

typedef struct
{
    uint8_t mode_hid;
    uint8_t mode_led;
    uint8_t led_r;
    uint8_t led_g;
    uint8_t led_b;
} sys_param_t;

esp_err_t settings_read_parameter_from_nvs(void);
esp_err_t settings_write_parameter_to_nvs(void);
sys_param_t *settings_get_parameter(void);
