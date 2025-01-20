/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "esp_log.h"
#include "esp_check.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "rgb_matrix_types.h"
#include "rgb_matrix.h"

#define NAME_SPACE "sys_param"
#define KEY "rgb_matrix"

static const char *TAG = "rgb_matrix_nvs";

esp_err_t nvs_init_rgb_matrix()
{
    nvs_handle_t my_handle = 0;
    esp_err_t ret = nvs_open(NAME_SPACE, NVS_READONLY, &my_handle);
    if (ESP_ERR_NVS_NOT_FOUND == ret) {
        ESP_LOGW(TAG, "NVS not found");
        return ESP_OK;
    }

    ESP_GOTO_ON_FALSE(ESP_OK == ret, ret, err, TAG, "nvs open failed (0x%x)", ret);

    size_t len = sizeof(rgb_config_t);
    ret = nvs_get_blob(my_handle, KEY, &rgb_matrix_config, &len);
    ESP_GOTO_ON_FALSE(ESP_OK == ret, ret, err, TAG, "can't read param");
    nvs_close(my_handle);

    return ret;
err:
    if (my_handle) {
        nvs_close(my_handle);
    }
    return ret;
}

esp_err_t nvs_flush_rgb_matrix(bool if_flush)
{
    if (if_flush) {
        ESP_LOGI(TAG, "Saving settings");
        nvs_handle_t my_handle = {0};
        esp_err_t err = nvs_open(NAME_SPACE, NVS_READWRITE, &my_handle);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        } else {
            err = nvs_set_blob(my_handle, KEY, &rgb_matrix_config, sizeof(rgb_config_t));
            err |= nvs_commit(my_handle);
            nvs_close(my_handle);
        }
        return ESP_OK == err ? ESP_OK : ESP_FAIL;
    }
    return ESP_OK;
}
