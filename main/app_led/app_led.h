#ifndef APP_LED_H_
#define APP_LED_H_

#include "esp_err.h"

void appLedStart(void);
esp_err_t bspWs2812Enable(bool enable);

#endif /* APP_LED_H_ */