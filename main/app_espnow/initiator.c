#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include "esp_mac.h"
#include "esp_random.h"

#include "espnow.h"
#include "espnow_storage.h"
#include "espnow_utils.h"

#include "espnow_prov.h"

#include "espnow_security.h"
#include "espnow_security_handshake.h"

static const char *TAG = "espnow_initiator";

#define CONFIG_APP_ESPNOW_SESSION_POP "espnow_pop"
const char *pop_data = CONFIG_APP_ESPNOW_SESSION_POP;
static TaskHandle_t s_sec_task;

static esp_err_t app_espnow_prov_responder_recv_cb(uint8_t *src_addr, void *data,
                                                   size_t size, wifi_pkt_rx_ctrl_t *rx_ctrl)
{
    ESP_PARAM_CHECK(src_addr);
    ESP_PARAM_CHECK(data);
    ESP_PARAM_CHECK(size);
    ESP_PARAM_CHECK(rx_ctrl);
    espnow_prov_initiator_t *initiator_info = (espnow_prov_initiator_t *)data;
    ESP_LOGI(TAG, "MAC: " MACSTR ", Channel: %d, RSSI: %d, Product_id: %s, Device Name: %s, Auth Mode: %d, device_secret: %s",
             MAC2STR(src_addr), rx_ctrl->channel, rx_ctrl->rssi,
             initiator_info->product_id, initiator_info->device_name,
             initiator_info->auth_mode, initiator_info->device_secret);
    return ESP_OK;
}

/// @brief 通过ESPNOW广播设备信息，等待被配对设备扫描到
/// @param sec 
/// @return
esp_err_t app_espnow_prov_beacon_start(int32_t sec)
{
    wifi_config_t wifi_config = {0};
    espnow_prov_wifi_t prov_wifi_config = {0};
    espnow_prov_responder_t responder_info = {
        .product_id = "responder_test",
    };

    esp_err_t ret = esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Get wifi config failed, %d", ret);
        return ret;
    }

    if (strlen((const char *)wifi_config.sta.ssid) == 0)
    {
        ESP_LOGW(TAG, "WiFi not configured");
        return ESP_FAIL;
    }

    memcpy(&prov_wifi_config.sta, &wifi_config.sta, sizeof(wifi_sta_config_t));

    // 该函数会创建一个定时器，定时发送广播包，等待被配对设备扫描到
    // app_espnow_prov_responder_recv_cb 处理配对设备的回复
    ret = espnow_prov_responder_start(&responder_info, pdMS_TO_TICKS(sec * 1000), &prov_wifi_config, app_espnow_prov_responder_recv_cb);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "espnow_prov_responder_start failed, %d", ret);
    }

    return ret;
}

static void app_espnow_initiator_sec_task(void *arg)
{
    espnow_sec_result_t espnow_sec_result = {0};
    espnow_sec_responder_t *info_list = NULL;
    espnow_addr_t *dest_addr_list = NULL;
    size_t num = 0;
    uint8_t key_info[APP_KEY_LEN];

    if (espnow_get_key(key_info) != ESP_OK)
    {
        esp_fill_random(key_info, APP_KEY_LEN);
    }
    espnow_set_key(key_info);
    espnow_set_dec_key(key_info);

    uint32_t start_time1 = xTaskGetTickCount();
    espnow_sec_initiator_scan(&info_list, &num, pdMS_TO_TICKS(3000));
    ESP_LOGW(TAG, "espnow wait security num: %u", num);

    if (num == 0)
    {
        goto EXIT;
    }

    dest_addr_list = ESP_MALLOC(num * ESPNOW_ADDR_LEN);

    for (size_t i = 0; i < num; i++)
    {
        memcpy(dest_addr_list[i], info_list[i].mac, ESPNOW_ADDR_LEN);
    }

    espnow_sec_initiator_scan_result_free();

    uint32_t start_time2 = xTaskGetTickCount();
    esp_err_t ret = espnow_sec_initiator_start(key_info, pop_data, dest_addr_list, num, &espnow_sec_result);
    ESP_ERROR_GOTO(ret != ESP_OK, EXIT, "<%s> espnow_sec_initiator_start", esp_err_to_name(ret));

    ESP_LOGI(TAG, "App key is sent to the device to complete, Spend time: %" PRId32 " ms, Scan time: %" PRId32 "ms",
             (xTaskGetTickCount() - start_time1) * portTICK_PERIOD_MS,
             (start_time2 - start_time1) * portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Devices security completed, successed_num: %u, unfinished_num: %u",
             espnow_sec_result.successed_num, espnow_sec_result.unfinished_num);

EXIT:
    ESP_FREE(dest_addr_list);
    espnow_sec_initiator_result_free(&espnow_sec_result);

    vTaskDelete(NULL);
    s_sec_task = NULL;
}

void app_espnow_initiator()
{
    if (!s_sec_task)
        xTaskCreate(app_espnow_initiator_sec_task, "sec", 3072, NULL, tskIDLE_PRIORITY + 1, &s_sec_task);
}