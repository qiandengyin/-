#include "esp_log.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "hal/gpio_types.h"
#include "esp_adc/adc_continuous.h"
#include "bsp_rocker.h"


#define ROCKER_X_PIN 10
#define ROCKER_Y_PIN 11

/// @brief 摇杆初始化
/// @param  
void bsp_rocker_init(void)
{
    gpio_config_t io_conf = {0};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = ((1ULL << ROCKER_X_PIN) | (1ULL << ROCKER_Y_PIN));
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
}
