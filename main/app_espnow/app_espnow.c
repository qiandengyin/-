#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "espnow.h"
#include "espnow_utils.h"
#include "espnow_ctrl.h"
#include <wifi_provisioning/manager.h>

#include "app_wifi.h"
#include "initiator.h"
#include "app_espnow.h"

static const char *TAG = "app_espnow";

static espnow_frame_head_t sg_initiator_frame = {
    .retransmit_count = 10,
    .broadcast = true,
    .channel = ESPNOW_CHANNEL_ALL,
    .forward_ttl = 10,
    .forward_rssi = -25,
    .security = 0,
};

#define ESPNOW_CTRL_DATA_LEN 32
static espnow_ctrl_data_t *sg_initiator_ctrl_data = NULL;

/// @brief 十六进制数组转字符串
/// @param arr 
/// @param arr_size 
/// @param str 
/// @return
int hexArray_to_hexString(uint8_t *arr, int arr_size, char *str)
{
    int i;
    int str_len = 0;
    for (i = 0; i < arr_size; i++)
    {
        str_len += sprintf(str + str_len, "%02X", arr[i]);
    }
    str[str_len] = '\0'; // 确保字符串以 null 结尾
    return str_len;
}

/// @brief espnow向对端设备广播绑定请求
/// @param arg 
/// @param usr_data 
void app_espnow_bind(void)
{
    if (app_wifi_connected_already() != WIFI_STATUS_CONNECTED_OK)
    {
        ESP_LOGI(TAG, "wifi is connecting, not bind");
        return;
    }
    
    // 向对端设备广播绑定请求
    // 对端设备保存配置
    // 本机不保存配置
    ESP_LOGI(TAG, "initiator request bind");
    app_wifi_lock(0);
    espnow_ctrl_initiator_bind(ESPNOW_ATTRIBUTE_KEY_1, true);
    app_wifi_unlock();
}

/// @brief espnow向对端设备广播解绑请求
/// @param arg 
/// @param usr_data 
void app_espnow_unbind(void)
{
    if (app_wifi_connected_already() != WIFI_STATUS_CONNECTED_OK)
    {
        ESP_LOGI(TAG, "wifi is connecting, not unbind");
        return;
    }

    // 向对端设备广播解绑请求
    // 对端设备保存配置
    // 本机不保存配置
    ESP_LOGI(TAG, "initiator request unbind");
    app_wifi_lock(0);
    espnow_ctrl_initiator_bind(ESPNOW_ATTRIBUTE_KEY_1, false);
    app_wifi_unlock();
}

/// @brief ESPNOW发送数据
/// @param data 
/// @param data_len 
void app_espnow_send_data(uint8_t *data, size_t data_len)
{
    if (app_wifi_connected_already() != WIFI_STATUS_CONNECTED_OK)
    {
        return;
    }

    if (sg_initiator_ctrl_data == NULL)
    {
        ESP_LOGI(TAG, "sg_initiator_ctrl_data is NULL");
        return;
    }

    if ((data_len * 2 + 1) >= ESPNOW_CTRL_DATA_LEN)
    {
        ESP_LOGI(TAG, "data_len is too long");
        return;
    }

    // 按键状态改变, 发送数据
    uint8_t key_changed = 0;
    static uint8_t key_buffer_last[8] = {0};
    uint8_t key_buffer[8] = {0};
    memcpy(key_buffer, data, data_len);
    for (uint8_t i = 0; i < 8; i++)
    {
        if (key_buffer[i] != key_buffer_last[i])
            key_changed++;
    }
    if (key_changed == 0)
        return;
    memcpy(key_buffer_last, key_buffer, sizeof(key_buffer));

    sg_initiator_ctrl_data->initiator_attribute    = ESPNOW_ATTRIBUTE_KEY_1;
    sg_initiator_ctrl_data->responder_attribute    = ESPNOW_ATTRIBUTE_POWER;
    sg_initiator_ctrl_data->responder_value_s_flag = 0x000000;
    memset(sg_initiator_ctrl_data->responder_value_s, '\0', ESPNOW_CTRL_DATA_LEN);
    hexArray_to_hexString(data, data_len, sg_initiator_ctrl_data->responder_value_s);
    sg_initiator_ctrl_data->responder_value_s_size = ESPNOW_CTRL_DATA_LEN; // 这个是 responder_value_s 的大小
    app_wifi_lock(0);
    espnow_ctrl_send(ESPNOW_ADDR_BROADCAST, sg_initiator_ctrl_data, &sg_initiator_frame, pdMS_TO_TICKS(1000));
    app_wifi_unlock();
}

/// @brief ESPNOW广播配网信息
/// @param
void app_espnow_send_wifi_config(void)
{
    if (app_wifi_connected_already() != WIFI_STATUS_CONNECTED_OK)
    {
        ESP_LOGI(TAG, "wifi is not connected, not send wifi config");
        return;
    }

    bool enabled;
    espnow_get_config_for_data_type(ESPNOW_DATA_TYPE_PROV, &enabled);
    if (enabled)
    {
        ESP_LOGI(TAG, "WiFi provisioning over ESP-NOW is started");
    }
    else
    {
        ESP_LOGI(TAG, "Start WiFi provisioning over ESP-NOW on initiator");
        app_espnow_prov_beacon_start(30); // 广播配网信息30秒
    }
}

void app_espnow_init(void)
{
    size_t total_size = sizeof(espnow_ctrl_data_t) + ESPNOW_CTRL_DATA_LEN;
    sg_initiator_ctrl_data = (espnow_ctrl_data_t *)malloc(total_size);
    if (sg_initiator_ctrl_data == NULL)
        ESP_LOGI(TAG, "Memory allocation failed");
    espnow_config_t espnow_config = ESPNOW_INIT_CONFIG_DEFAULT();
    espnow_config.qsize = 32;
    espnow_config.sec_enable = 1;
    espnow_init(&espnow_config);
    app_espnow_initiator();
}
