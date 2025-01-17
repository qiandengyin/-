#ifndef _BSP_SPIFFS_H_
#define _BSP_SPIFFS_H_


#include <stdint.h>
#include "esp_err.h"

#define BSP_SPIFFS_MOUNT_POINT "/spiffs"

esp_err_t bsp_spiffs_mount(void);
esp_err_t bsp_spiffs_unmount(void);

#endif /* _BSP_SPIFFS_H_ */