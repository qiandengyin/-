#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp32s3/rom/ets_sys.h"
#include "hid_dev.h"
#include "bsp_keyboard.h"
#include "function_keys.h"
#include "keyboard.h"
#include "app_ble_hid.h"
#include "app_tusb_hid.h"
#include "app_espnow.h"
#include "app_udp_client.h"
#include "app_uart.h"


// 去抖动算法: https://www.kennethkuhn.com/electronics/debounce.c
// 6键无冲 6KRO, 6-Key Rollover
// 全键无冲 NKRO, N-Key Rollover

/** @brief 6键无冲报文
 * byte 0:
 *       bit 0: 左 Ctrl
 *       bit 1: 左 Shift
 *       bit 2: 左 Alt
 *       bit 3: 左 GUI
 *       bit 4: 右 Ctrl
 *       bit 5: 右 Shift
 *       bit 6: 右 Alt
 *       bit 7: 右 GUI
 * byte 1: 保留
 * byte 2: 非修饰键 按键码
 * byte 3: 非修饰键 按键码
 * byte 4: 非修饰键 按键码
 * byte 5: 非修饰键 按键码
 * byte 6: 非修饰键 按键码
 * byte 7: 非修饰键 按键码
*/
static uint8_t hidReportBuffer[8] = {0};

// 自定义按键
#define CUSTOM_KEY_FN  1000 // FN按键
#define CUSTOM_KEY_REC 1001 // 录音按键
#define CUSTOM_KEY_1 0      // 自定义按键1
#define CUSTOM_KEY_2 0      // 自定义按键2
#define ROCKER_KEY_X 0      // 摇杆X轴按键
#define ROCKER_KEY_Y 0      // 摇杆Y轴按键

// 81颗按键
#define IO_NUMBER (11 * 8)
uint8_t scanBuffer[IO_NUMBER / 8 + 1] = {0xff};
uint8_t debounceBuffer[IO_NUMBER / 8 + 1] = {0xff};
uint8_t remapBuffer[IO_NUMBER / 8 + 1] = {0xff};
int16_t keyMap[2][IO_NUMBER] = {
    // 键盘布局在移位寄存器上的位置
    {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
        26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13,
        27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41,
        54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65,
        73, 72, 71, 70, 69, 68, 67, 66,
        82, 83, 84,
        85, 86, 87,
        88, 89,
    },
    // 键盘布局: 82键
    {
        HID_KEY_ESCAPE, HID_KEY_F1, HID_KEY_F2, HID_KEY_F3, HID_KEY_F4, HID_KEY_F5, HID_KEY_F6, HID_KEY_F7, HID_KEY_F8, HID_KEY_F9, HID_KEY_F10, HID_KEY_F11, HID_KEY_F12,
        HID_KEY_GRV_ACCENT, HID_KEY_1, HID_KEY_2, HID_KEY_3, HID_KEY_4, HID_KEY_5, HID_KEY_6, HID_KEY_7, HID_KEY_8, HID_KEY_9, HID_KEY_0, HID_KEY_MINUS, HID_KEY_EQUAL, HID_KEY_DELETE,
        HID_KEY_TAB, HID_KEY_Q, HID_KEY_W, HID_KEY_E, HID_KEY_R, HID_KEY_T, HID_KEY_Y, HID_KEY_U, HID_KEY_I, HID_KEY_O, HID_KEY_P, HID_KEY_LEFT_BRKT, HID_KEY_RIGHT_BRKT, HID_KEY_BACK_SLASH,
        HID_KEY_CAPS_LOCK, HID_KEY_A, HID_KEY_S, HID_KEY_D, HID_KEY_F, HID_KEY_G, HID_KEY_H, HID_KEY_J, HID_KEY_K, HID_KEY_L, HID_KEY_SEMI_COLON, HID_KEY_SGL_QUOTE, HID_KEY_RETURN,
        HID_KEY_LEFT_SHIFT, HID_KEY_Z, HID_KEY_X, HID_KEY_C, HID_KEY_V, HID_KEY_B, HID_KEY_N, HID_KEY_M, HID_KEY_COMMA, HID_KEY_DOT, HID_KEY_FWD_SLASH, HID_KEY_RIGHT_SHIFT,
        HID_KEY_LEFT_CTRL, HID_KEY_LEFT_GUI, HID_KEY_LEFT_ALT, HID_KEY_SPACEBAR, CUSTOM_KEY_FN, HID_KEY_RIGHT_ALT, CUSTOM_KEY_REC, HID_KEY_RIGHT_CTRL,
        // -------------------------小键盘---------------------
        CUSTOM_KEY_1,       HID_KEY_UP_ARROW,   CUSTOM_KEY_2,
        HID_KEY_LEFT_ARROW, HID_KEY_DOWN_ARROW, HID_KEY_RIGHT_ARROW,
        // -------------------------摇杆-----------------------
        ROCKER_KEY_X,       ROCKER_KEY_Y,
    },
};

/// @brief 获取按键状态
/// @param keyIndex 
/// @param bitIndex 
/// @return
uint8_t keyboardGetKeyState(uint8_t keyIndex, uint8_t bitIndex)
{
    return remapBuffer[keyIndex] & (0x80 >> bitIndex);
}

/***************************************************************************
 * 从移位寄存器映射到键盘布局
***************************************************************************/

/// @brief 按键映射
/// @param  
static void keyboardRemap(void)
{
    int16_t index, bitIndex;
    memset(remapBuffer, 0, IO_NUMBER / 8);
    for (int16_t i = 0; i < (IO_NUMBER / 8); i++)
    {
        for (int16_t j = 0; j < 8; j++)
        {
            // 从 keyMap[0] 中取出某个按键在移位寄存器上的位置
            index = (int16_t)(keyMap[0][i * 8 + j] / 8);
            bitIndex = (int16_t)(keyMap[0][i * 8 + j] % 8);
            // 判断scanBuffer中该位置是否被按下
            // remapBuffer从索引0开始,每字节从高到低依次对应keyMap[1]的按键: 未按下置1, 否则置0
            if (scanBuffer[index] & (0x80 >> bitIndex))
                remapBuffer[i] |= (0x80 >> j);
        }
        // 按位取反,将按下的键置1
        remapBuffer[i] = ~remapBuffer[i];
    }
}

static void printRemapBuffer(void)
{
    printf("\r\n");
    for (int16_t i = 0; i < IO_NUMBER / 8; i++)
    {
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if ((remapBuffer[i] << bit) & 0x80)
                printf("1");
            else
                printf("0");
        }
    }
}

/***************************************************************************
 * 扫描移位寄存器
***************************************************************************/

/// @brief 扫描按键
/// @param  
static void ScanKeyStates(void)
{
    memset(scanBuffer, 0xFF, sizeof(scanBuffer) / sizeof(scanBuffer[0]));
    bsp_74hc165d_read(scanBuffer, sizeof(scanBuffer) / sizeof(scanBuffer[0]));
}

/// @brief 使用稚晖君瀚文固件中使用的对称延迟独立滤波
/// @param  
static void ApplyDebounceFilter(void)
{
    uint8_t mask;
    for (int i = 0; i < sizeof(scanBuffer) / sizeof(scanBuffer[0]); i++)
    {
        // 异或运算
        // 相同为0, 不同为1
        // 前一次和本次采样某位为0, 处于按下状态
        // 前一次和本次采样某位为1, 处于释放状态
        // 前一次和本次采样某位不同, 则该位置1(处于释放状态)
        mask = debounceBuffer[i] ^ scanBuffer[i];
        scanBuffer[i] |= mask;
    }
}

/// @brief 打印扫描数据
/// @param  
static void printScanBuffer(void)
{
    for (uint8_t i = 0; i < (sizeof(scanBuffer) / sizeof(scanBuffer[0])); i++)
    {
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if ((scanBuffer[i] << bit) & 0x80)
                printf("1");
            else
                printf("0");
        }
    }
    printf("\r\n");
}

/***************************************************************************
 * 编码HID报文
***************************************************************************/

/// @brief 编码键盘报文
/// @param  
static void keyToHidMessage(void)
{
    uint8_t key_count = 0;
    memset(hidReportBuffer, 0x00, sizeof(hidReportBuffer) / sizeof(hidReportBuffer[0]));
    for (int16_t i = 0; i < IO_NUMBER / 8; i++)
    {
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            int16_t index = i * 8 + bit;
            if (index == KEY_FN_INDEX)
                continue;
            if ((remapBuffer[i] << bit) & 0x80)
            {
                if (keyMap[1][index] == HID_KEY_LEFT_CTRL)
                    hidReportBuffer[0] |= 0x01; // 左 Ctrl
                else if (keyMap[1][index] == HID_KEY_LEFT_SHIFT)
                    hidReportBuffer[0] |= 0x02; // 左 Shift
                else if (keyMap[1][index] == HID_KEY_LEFT_ALT)
                    hidReportBuffer[0] |= 0x04; // 左 Alt
                else if (keyMap[1][index] == HID_KEY_LEFT_GUI)
                    hidReportBuffer[0] |= 0x08; // 左 GUI
                else if (keyMap[1][index] == HID_KEY_RIGHT_CTRL)
                    hidReportBuffer[0] |= 0x10; // 右 Ctrl
                else if (keyMap[1][index] == HID_KEY_RIGHT_SHIFT)
                    hidReportBuffer[0] |= 0x20; // 右 Shift
                else if (keyMap[1][index] == HID_KEY_RIGHT_ALT)
                    hidReportBuffer[0] |= 0x40; // 右 Alt
                else
                {
                    hidReportBuffer[2 + key_count] = keyMap[1][index];
                    key_count++;
                    if (key_count >= 6)
                        return;
                }
            }
        }
    }
}

/***************************************************************************
 * 键盘任务
***************************************************************************/
static void keyboardTask(void *arg)
{
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        ScanKeyStates();
        memcpy(debounceBuffer, scanBuffer, sizeof(scanBuffer) / sizeof(scanBuffer[0]));
        ets_delay_us(100);
        ScanKeyStates();
        ApplyDebounceFilter();
        keyboardRemap();
        keyToHidMessage();
        // printScanBuffer(); // 打印扫描到的键值
        printRemapBuffer(); // 打印映射后的键值
        if (functionKeys())
            continue;
        shutdownByFn();
        // 发送HID报文
        switch (appUartGetHidMode())
        {
        case MODE_HID_USB:
            app_tusb_hid_send_key(hidReportBuffer, sizeof(hidReportBuffer) / sizeof(hidReportBuffer[0]));
            break;
        case MODE_HID_BLE:
            app_ble_hid_send_key(hidReportBuffer, sizeof(hidReportBuffer) / sizeof(hidReportBuffer[0]));
            break;
        case MODE_HID_ESPNOW:
            app_espnow_send_data(hidReportBuffer, sizeof(hidReportBuffer) / sizeof(hidReportBuffer[0]));
            break;
        case MODE_HID_UDP:
            app_udp_client_send_data(hidReportBuffer, sizeof(hidReportBuffer) / sizeof(hidReportBuffer[0]));
            break;
        default:
            break;
        }
    }
    vTaskDelete(NULL);
}

void keyboardStart(void)
{
    xTaskCreate(&keyboardTask, "keyboardTask", 1024 * 6, NULL, 7, NULL);
}
