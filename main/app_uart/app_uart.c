#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

#include "rgb_matrix.h"

#include "bsp_keyboard.h"
#include "app_uart.h"

static const int RX_BUF_SIZE = 1024;

#define TXD_PIN (GPIO_NUM_14)
#define RXD_PIN (GPIO_NUM_21)

// int sendData(const char *logName, const char *data)
// {
//     const int len = strlen(data);
//     const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
//     return txBytes;
// }

// static void app_uart_tx_task(void *arg)
// {
//     static const char *TX_TASK_TAG = "TX_TASK";
//     esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
//     while (1)
//     {
//         sendData(TX_TASK_TAG, "Hello world");
//         vTaskDelay(2000 / portTICK_PERIOD_MS);
//     }
// }

/************************************************************************
 * HID模式
************************************************************************/

/// @brief 设置HID模式
/// @param cmd 
void appUartSetHidMode(uint8_t cmd)
{
    sys_param_t *param = settings_get_parameter();
    switch (cmd)
    {
    case 0x11:
        param->mode_hid = MODE_HID_USB;
        break;
    case 0x12:
        param->mode_hid = MODE_HID_BLE;
        break;
    case 0x13:
        param->mode_hid = MODE_HID_ESPNOW;
        break;
    case 0x14:
        param->mode_hid = MODE_HID_UDP;
        break;
    default:
        break;
    }
}

/// @brief 获取HID模式
/// @param  
/// @return 
uint8_t appUartGetHidMode(void)
{
    sys_param_t *param = settings_get_parameter();
    return param->mode_hid;
}

/************************************************************************
 * LED模式
************************************************************************/

void appUartSetLedMode(uint8_t cmd)
{
    uint16_t index = 1;
    sys_param_t *param = settings_get_parameter();
    switch (cmd)
    {
    case 0x22:
        index = (rgb_matrix_get_mode() + 1) % RGB_MATRIX_EFFECT_MAX;
        rgb_matrix_mode(index);
        param->mode_led = index;
        break;
    case 0x23:
        index = rgb_matrix_get_mode() - 1;
        if (index < 1)
            index = RGB_MATRIX_EFFECT_MAX - 1;
        rgb_matrix_mode(index);
        param->mode_led = index;
        break;
    default:
        break;
    }
}

/************************************************************************
 * uart任务
************************************************************************/

static void app_uart_rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
    while (1)
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 100 / portTICK_PERIOD_MS);
        if (rxBytes <= 0)
            continue;
        data[rxBytes] = 0;
        if (rxBytes != 8)
            continue;
        if (data[0] != 0xAA && data[1] != 0x55 && data[6] != 0x55 && data[7] != 0xAA)
            continue;
        ESP_LOGI(RX_TASK_TAG, "Read %d", rxBytes);
        ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        
        if (data[2] >= 0x11 && data[2] <= 0x14)
        {
            appUartSetHidMode(data[2]);
        }
        else if (data[2] == 0x21)
        {
            rgb_matrix_sethsv(data[3], data[4], data[5]);
        }
        else if (data[2] >= 0x22 && data[2] <= 0x23)
        {
            // appUartSetLedMode(data[2]);
        }
        else if (data[2] == 0x31)
        {
            if (data[3] == 0x01)
            {
                bsp_audio_mute_enable(false);
            }
            else
            {
                bsp_audio_mute_enable(true);
            }
        }
        settings_write_parameter_to_nvs();
    }
    free(data);
}

void app_uart_init(void)
{
    settings_read_parameter_from_nvs();

    const uart_config_t uart_config = {
        .baud_rate  = 115200,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    xTaskCreate(app_uart_rx_task, "uart_rx_task", 1024 * 4, NULL, 3, NULL);
    // xTaskCreate(app_uart_tx_task, "uart_tx_task", 1024 * 3, NULL, 6, NULL);
}
