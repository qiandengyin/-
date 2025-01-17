#ifndef KEYBOARD_BSP_H_
#define KEYBOARD_BSP_H_

#include "bsp_74hc165.h"
#include "bsp_rocker.h"
#include "bsp_spiffs.h"
#include "bsp_audio.h"
#include "bsp_power.h"

esp_err_t bsp_keyboard_init(void);

#endif /* KEYBOARD_BSP_H_ */