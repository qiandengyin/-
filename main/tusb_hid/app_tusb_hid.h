#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void app_tusb_hid_init(void);
void app_tusb_hid_send_key(uint8_t *keyBuf, uint8_t len);

#ifdef __cplusplus
}
#endif