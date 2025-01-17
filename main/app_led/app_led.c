#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "hal/spi_hal.h"
#include "esp_log.h"

#include "app_led.h"

#include "led_strip.h"
#include "rgb_matrix_drivers.h"
#include "rgb_matrix.h"

static const char *TAG = "app_led";

#define KBD_WS2812_POWER_IO 5
#define LIGHTMAP_GPIO       38
#define LIGHTMAP_NUM        CONFIG_MATRIX_LED_COUNT

static led_strip_handle_t s_led_strip = NULL;
static bool s_led_enable = false;

static TaskHandle_t appLedTaskHandle = NULL;

// https://docs.qmk.fm/features/rgb_matrix
// x = 224 / (NUMBER_OF_COLS - 1) * COL_POSITION [0,15]
// y =  64 / (NUMBER_OF_ROWS - 1) * ROW_POSITION [0,5]

led_config_t g_led_config = {
    {
        // Key Matrix to LED Index
        {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 80, 89, 90},
        {26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 81, 91},
        {27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 82, 92},
        {53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 83, 88, 93},
        {54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 74, 75, 76, 94},
        {73, 72, 71, 70, 69, 68, 67, 66, 79, 78, 77, 84, 85, 86, 87, 95},
    },
    {
        // LED Index to Physical Position
        {0, 0}, {15, 0}, {30, 0}, {45, 0}, {60, 0}, {75, 0}, {90, 0}, {105, 0}, {119, 0}, {134, 0}, {149, 0}, {164, 0}, {179, 0}, {194, 0}, {209, 0}, {224, 0}, // 0-15
        {0, 13},{15, 13},{30, 13},{45, 13},{60, 13},{75, 13},{90, 13},{105, 13},{119, 13},{134, 13},{149, 13},{164, 13},{179, 13},{194, 13},{209, 13},{224, 13},// 16-31
        {0, 26},{15, 26},{30, 26},{45, 26},{60, 26},{75, 26},{90, 26},{105, 26},{119, 26},{134, 26},{149, 26},{164, 26},{179, 26},{194, 26},{209, 26},{224, 26},// 32-47
        {0, 39},{15, 39},{30, 39},{45, 39},{60, 39},{75, 39},{90, 39},{105, 39},{119, 39},{134, 39},{149, 39},{164, 39},{179, 39},{194, 39},{209, 39},{224, 39},// 48-63
        {0, 52},{15, 52},{30, 52},{45, 52},{60, 52},{75, 52},{90, 52},{105, 52},{119, 52},{134, 52},{149, 52},{164, 52},{179, 52},{194, 52},{209, 52},{224, 52},// 64-79
        {0, 64},{15, 64},{30, 64},{45, 64},{60, 64},{75, 64},{90, 64},{105, 64},{119, 64},{134, 64},{149, 64},{164, 64},{179, 64},{194, 64},{209, 64},{224, 64},// 80-95
    },
    {
        // LED Index to Flag
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,// 0-15
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,// 16-31
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,// 32-47
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,// 48-63
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,// 64-79
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,// 80-95
    }
};

static esp_err_t bspWs2812Init(led_strip_handle_t *led_strip)
{
    if (s_led_strip)
    {
        if (led_strip)
        {
            *led_strip = s_led_strip;
        }
        return ESP_OK;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << KBD_WS2812_POWER_IO,
        .mode = GPIO_MODE_OUTPUT_OD,
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);

    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = LIGHTMAP_GPIO,          // The GPIO that connected to the LED strip's data line
        .max_leds = LIGHTMAP_NUM,                 // The number of LEDs in the strip,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
        .led_model = LED_MODEL_WS2812,            // LED strip model
        .flags.invert_out = false,                // whether to invert the output signal (useful when your hardware has a level inverter)
    };

    // LED strip backend configuration: SPI
    led_strip_spi_config_t spi_config = {
        .clk_src = SPI_CLK_SRC_XTAL, // different clock source can lead to different power consumption
        .flags.with_dma = true,      // Using DMA can improve performance and help drive more LEDs
        .spi_bus = SPI3_HOST,        // SPI bus ID
    };

    // LED Strip object handle
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &s_led_strip));

    if (led_strip)
    {
        *led_strip = s_led_strip;
    }
    return ESP_OK;
}

esp_err_t bspWs2812Enable(bool enable)
{
    if (s_led_enable == enable)
    {
        return ESP_OK;
    }
    
    if (!enable)
    {
        gpio_hold_dis(KBD_WS2812_POWER_IO);
    }
    gpio_set_level(KBD_WS2812_POWER_IO, !enable);
    /*!< Make output stable in light sleep */
    if (enable)
    {
        gpio_hold_en(KBD_WS2812_POWER_IO);
    }
    s_led_enable = enable;
    return ESP_OK;
}

esp_err_t bspWs2812Clear(void)
{
    return led_strip_clear(s_led_strip);
}

bool bspWs2812IsEnable(void)
{
    return s_led_enable;
}

esp_err_t bspRgbMatrixInit(void)
{
    if (!s_led_strip)
    {
        bspWs2812Init(NULL);
    }
    rgb_matrix_driver_init(s_led_strip, LIGHTMAP_NUM);
    rgb_matrix_init();
    return ESP_OK;
}

static void appLedTask(void *arg)
{
    /*!< Init LED and clear WS2812's status */
    led_strip_handle_t led_strip = NULL;
    bspWs2812Init(&led_strip);
    if (led_strip)
    {
        led_strip_clear(led_strip);
    }
    bspRgbMatrixInit();

    bspWs2812Enable(true);

    uint16_t index = rgb_matrix_get_mode();
    ESP_LOGI(TAG, "Current RGB Matrix mode: %d", index);
    if (index == RGB_MATRIX_NONE)
        index = 1;
    rgb_matrix_mode(index);
    ESP_LOGI(TAG, "RGB_MATRIX_EFFECT_MAX: %d", RGB_MATRIX_EFFECT_MAX);

    while (1)
    {
        // static int index = 0;
        // led_strip_set_pixel(led_strip, index, 0, 255, 0);
        // led_strip_refresh(led_strip);
        // index++;
        // if (index >= LIGHTMAP_NUM)
        // {
        //     vTaskDelay(1000 / portTICK_PERIOD_MS);
        //     led_strip_clear(led_strip);
        //     vTaskDelay(1000 / portTICK_PERIOD_MS);
        //     index = 0;
        //     continue;
        // }
        // vTaskDelay(1000 / portTICK_PERIOD_MS);

        if (bspWs2812IsEnable())
        {
            rgb_matrix_task();
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void appLedStart(void)
{
    xTaskCreate(appLedTask, "appLedTask", 4 * 1024, NULL, 3, &appLedTaskHandle);
}
