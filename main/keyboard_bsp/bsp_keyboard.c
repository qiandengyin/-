
#include "esp_log.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "hal/gpio_types.h"
#include "driver/i2c.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"

#include "bsp_keyboard.h"

#define TAG "BSP_KEYBOARD"

esp_err_t bsp_keyboard_init(void)
{
    esp_err_t ret = ESP_OK;
    ESP_LOGD(TAG, "Keyboard init");
    ESP_ERROR_CHECK(bsp_spiffs_mount());
   // ESP_ERROR_CHECK(bsp_i2c_init());
   // ESP_ERROR_CHECK(bsp_audio_init());

    bsp_74hc165d_init();
    return ret;
}
