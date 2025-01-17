#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_hidd_prf_api.h"

void app_ble_hid_init(void);

void app_ble_hid_send_key(uint8_t *keyBuf, uint8_t len);

#ifdef __cplusplus
}
#endif