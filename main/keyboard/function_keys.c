#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp32s3/rom/ets_sys.h"
#include "audio_player.h"
#include "rgb_matrix.h"
#include "app_led.h"
#include "bsp_keyboard.h"
#include "function_keys.h"
#include "keyboard.h"
#include "app_espnow.h"
#include "app_uart.h"

enum {
    SPECIAL_KEY_CUSTOM_LEFT = 0,
    SPECIAL_KEY_CUSTOM_RIGHT,
    SPECIAL_KEY_UPARROW,
    SPECIAL_KEY_DOWNARROW,
    SPECIAL_KEY_RIGHTARROW,
    SPECIAL_KEY_LEFTARROW,
    SPECIAL_KEY_MAX,
};

/// @brief REC键
/// @param  
/// @return 
uint8_t getRecKey(void)
{
    uint8_t index = KEY_REC_INDEX / 8;
    uint8_t bitIndex = KEY_REC_INDEX % 8;
    return keyboardGetKeyState(index, bitIndex);
}

/// @brief FN键
/// @param  
/// @return 
static uint8_t getFnKey(void)
{
    uint8_t index = KEY_FN_INDEX / 8;
    uint8_t bitIndex = KEY_FN_INDEX % 8;
    return keyboardGetKeyState(index, bitIndex);
}

/// @brief 自定义键: 左键
/// @param  
/// @return 
static uint8_t getCustomLeftKey(void)
{
    uint8_t index = KEY_CUSTOM_LEFT_INDEX / 8;
    uint8_t bitIndex = KEY_CUSTOM_LEFT_INDEX % 8;
    return keyboardGetKeyState(index, bitIndex);
}

/// @brief 自定义键: 右键
/// @param  
/// @return 
static uint8_t getCustomRightKey(void)
{
    uint8_t index = KEY_CUSTOM_RIGHT_INDEX / 8;
    uint8_t bitIndex = KEY_CUSTOM_RIGHT_INDEX % 8;
    return keyboardGetKeyState(index, bitIndex);
}

/// @brief 箭头键: 上
/// @param  
/// @return 
static uint8_t getUpArrowKey(void)
{
    uint8_t index = KEY_UPARROW_INDEX / 8;
    uint8_t bitIndex = KEY_UPARROW_INDEX % 8;
    return keyboardGetKeyState(index, bitIndex);
}

/// @brief 箭头键: 下
/// @param  
/// @return 
static uint8_t getDownArrowKey(void)
{
    uint8_t index = KEY_DOWNARROW_INDEX / 8;
    uint8_t bitIndex = KEY_DOWNARROW_INDEX % 8;
    return keyboardGetKeyState(index, bitIndex);
}

/// @brief 箭头键: 右
/// @param  
/// @return 
static uint8_t getRightArrowKey(void)
{
    uint8_t index = KEY_RIGHTARROW_INDEX / 8;
    uint8_t bitIndex = KEY_RIGHTARROW_INDEX % 8;
    return keyboardGetKeyState(index, bitIndex);
}

/// @brief 箭头键: 左
/// @param  
/// @return 
static uint8_t getLeftArrowKey(void)
{
    uint8_t index = KEY_LEFTARROW_INDEX / 8;
    uint8_t bitIndex = KEY_LEFTARROW_INDEX % 8;
    return keyboardGetKeyState(index, bitIndex);
}

/// @brief fn + custom left
/// @param  
/// @return 
static uint8_t getFnCustomLeftKey(void)
{
    if (getFnKey() && getCustomLeftKey())
        return 1;
    return 0;
}

/// @brief fn + custom right
/// @param  
/// @return 
static uint8_t getFnCustomRightKey(void)
{
    if (getFnKey() && getCustomRightKey())
        return 1;
    return 0;
}

/// @brief fn + up arrow
/// @param  
/// @return 
static uint8_t getFnUpArrowKey(void)
{
    if (getFnKey() && getUpArrowKey())
        return 1;
    return 0;
}

/// @brief fn + down arrow
/// @param  
/// @return 
static uint8_t getFnDownArrowKey(void)
{
    if (getFnKey() && getDownArrowKey())
        return 1;
    return 0;
}

/// @brief FN + right arrow
/// @param  
/// @return 
static uint8_t getFnRightArrowKey(void)
{
    if (getFnKey() && getRightArrowKey())
        return 1;
    return 0;
}

/// @brief FN + left arrow
/// @param  
/// @return 
static uint8_t getFnLeftArrowKey(void)
{
    if (getFnKey() && getLeftArrowKey())
        return 1;
    return 0;
}

/// @brief 特殊键: 获取按键状态
static uint8_t (*specialKeyFunctions[SPECIAL_KEY_MAX])(void) = {
    getFnCustomLeftKey,
    getFnCustomRightKey,
    getFnUpArrowKey,
    getFnDownArrowKey,
    getFnRightArrowKey,
    getFnLeftArrowKey,
};

#define DEBOUNCE_TIME 50  // 去抖动时间（毫秒）
static uint32_t lastDebounceTime[SPECIAL_KEY_MAX] = {0};
static uint8_t lastButtonState[SPECIAL_KEY_MAX] = {0};
static uint8_t buttonState[SPECIAL_KEY_MAX] = {0};

/// @brief 特殊键
/// @param keyIndex 
static int functionKeysClick(uint8_t keyIndex)
{
    if (keyIndex >= SPECIAL_KEY_MAX)
        return -1;
    uint8_t reading =  specialKeyFunctions[keyIndex]();
    uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (reading != lastButtonState[keyIndex])
        lastDebounceTime[keyIndex] = currentTime;
    if ((currentTime - lastDebounceTime[keyIndex]) > DEBOUNCE_TIME)
    {
        // 如果按键状态已经稳定足够长的时间，则更新按键状态
        if (reading != buttonState[keyIndex])
        {
            buttonState[keyIndex] = reading;
            if (!buttonState[keyIndex])
                return -1;
            switch (keyIndex)
            {
            case SPECIAL_KEY_CUSTOM_LEFT:
            {
                uint16_t index = rgb_matrix_get_mode() - 1;
                if (index < 3)
                    index = 15;
                rgb_matrix_mode(index);
                printf("rgb_matrix_mode - : %d\r\n", index);
            }
            break;
            case SPECIAL_KEY_CUSTOM_RIGHT:
            {
                uint16_t index = rgb_matrix_get_mode() + 1;
                if (index > 15)
                    index = 3;
                rgb_matrix_mode(index);
                printf("rgb_matrix_mode + : %d\r\n", index);
            }
            break;
            case SPECIAL_KEY_UPARROW:
                if (appUartGetHidMode() == MODE_HID_ESPNOW)
                    app_espnow_bind();
                break;
            case SPECIAL_KEY_DOWNARROW:
                if (appUartGetHidMode() == MODE_HID_ESPNOW)
                    app_espnow_unbind();
                break;
            case SPECIAL_KEY_RIGHTARROW:
                if (appUartGetHidMode() == MODE_HID_ESPNOW)
                    app_espnow_send_wifi_config();
                break;
            case SPECIAL_KEY_LEFTARROW:
                printf("SPECIAL_KEY_LEFTARROW\r\n");
                break;
            default:
                break;
            }
        }
    }
    lastButtonState[keyIndex] = reading;
    return ((reading == 1) ? keyIndex : -1);
}

/// @brief 检查FN键功能
/// @param  
/// @return 
uint8_t functionKeys(void)
{
    uint8_t click_count = 0;
    for (uint8_t i = 0; i < SPECIAL_KEY_MAX; i++)
    {
        int keyIndex = functionKeysClick(i);
        if (keyIndex >= 0)
            click_count++;
    }
    return click_count;
}

/***************************************************************************
 * 长按FN键关机
***************************************************************************/
static uint32_t fnPressedTime = 0xffffffff;
static uint8_t  shutdownState = 0;
static uint8_t  bootState     = 0;

void shutdownByFn(void)
{
    bootState++;
    if (bootState < 100)
        return;
    bootState = 100;

    if (getFnKey())
    {
        if (fnPressedTime == 0)
        {
            fnPressedTime = xTaskGetTickCount();
        }
        else if (fnPressedTime != 0xffffffff)
        {
            if (xTaskGetTickCount() - fnPressedTime > 2000)
            {
                bspWs2812Enable(false);
                if (!bsp_audio_mute_is_enable())
                {
                    FILE *fp = fopen("/spiffs/powerOff.wav", "r");
                    if (fp)
                        audio_player_play(fp);
                }
                fnPressedTime = 0xffffffff;
                shutdownState = 1;
            }
        }
    }
    else
    {
        fnPressedTime = 0;
    }

    if (shutdownState && !getFnKey() && audio_player_get_state() == AUDIO_PLAYER_STATE_IDLE)
    {
        shutdownState = 0;
        bsp_power_off();
    }
}