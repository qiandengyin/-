#include "hal/gpio_types.h"
#include "driver/gpio.h"
#include "audio_player.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp_power.h"

#define MOTOR_POWER_PIN 39
#define POWER_PIN 45
void bsp_power_init(void)
{
    // zero-initialize the config structure.
    gpio_config_t io_conf = {0};
    // disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = ((1ULL << POWER_PIN) | (1ULL << MOTOR_POWER_PIN));
    // disable pull-down mode
    io_conf.pull_down_en = 0;
    // disable pull-up mode
    io_conf.pull_up_en = 0;
    // configure GPIO with the given settings
    gpio_config(&io_conf);

    gpio_set_level(POWER_PIN, 1);
    gpio_set_level(MOTOR_POWER_PIN, 0);
}

void bsp_power_off(void)
{
    gpio_set_level(MOTOR_POWER_PIN, 1);
    gpio_set_level(POWER_PIN, 0);
}