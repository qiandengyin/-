#ifndef _APP_ESPNOW_H_
#define _APP_ESPNOW_H_

#include <stdint.h>

void app_espnow_bind(void);
void app_espnow_unbind(void);
void app_espnow_send_data(uint8_t *data, size_t data_len);
void app_espnow_send_wifi_config(void);
void app_espnow_init(void);

#endif /* _APP_ESPNOW_H_ */
