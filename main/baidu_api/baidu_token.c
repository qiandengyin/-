#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "json_utils.h"

#include "app_wifi.h"
#include "baidu_api.h"

#define TAG "BaiduToken"

// 百度语音识别的API Key和Secret Key
#define API_KEY    "ADLeyE1z7Kti6IY4QAc24bVO"
#define SECRET_KEY "qOxqPBMp3kkqU13WXQzXCUzmmOtaGhj8"

#define BAIDU_URI_LENGTH    (200)
#define BAIDU_AUTH_ENDPOINT "https://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials"

static char *sg_access_token = NULL;
static char sg_mac_str[32] = {'\0'};
static bool token_updated_flag = false;

/// @brief 获取 access_token
/// @param  
/// @return 
void baidu_update_access_token(void)
{
    if (token_updated_flag)
    {
        return;
    }
    
    // 获取芯片 MAC 地址
    uint8_t mac_hex[6];
    esp_read_mac(mac_hex, ESP_MAC_WIFI_STA);
    for (int i = 0; i < sizeof(mac_hex); i++)
        sprintf(sg_mac_str + (i * 2), "%02X", mac_hex[i]);
    ESP_LOGI(TAG, "Baidu cuid = %s", sg_mac_str);
    
    // 获取百度云 access_token
    char *token = NULL;
    char *url = calloc(1, BAIDU_URI_LENGTH);
    if (url == NULL)
    {
        ESP_LOGE(TAG, "Memory allocation failed");
        return;
    }

    snprintf(url, BAIDU_URI_LENGTH, BAIDU_AUTH_ENDPOINT "&client_id=%s&client_secret=%s", API_KEY, SECRET_KEY);

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = NULL,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    esp_http_client_handle_t http_client = esp_http_client_init(&config);
    if (http_client == NULL)
    {
        ESP_LOGE(TAG, "Error creating http client");
        free(url);
        return;
    }
    if (esp_http_client_open(http_client, 0) != ESP_OK)
    {
        ESP_LOGE(TAG, "Error open http request to baidu auth server");
        goto _exit;
    }
    esp_http_client_fetch_headers(http_client);
    int max_len = 2 * 1024;
    char *data = malloc(max_len);
    if (data == NULL)
    {
        ESP_LOGE(TAG, "Memory allocation failed");
        goto _exit;
    }

    int read_index = 0, total_len = 0;
    while (1)
    {
        int read_len = esp_http_client_read(http_client, data + read_index, max_len - read_index);
        if (read_len <= 0)
        {
            break;
        }
        read_index += read_len;
        total_len += read_len;
        data[read_index] = 0;
    }

    if (total_len <= 0)
    {
        ESP_LOGE(TAG, "Invalid length of the response");
        free(data);
        goto _exit;
    }

    printf("%s: Data=%s", TAG, data);
    token = json_get_token_value(data, "access_token");
    free(data);
    if (token == NULL)
    {
        ESP_LOGE(TAG, "Invalid access_token");
        goto _exit;
    }

    if (sg_access_token)
        free(sg_access_token);
    int token_len = strlen(token) + 1;
    sg_access_token = malloc(token_len);
    if (sg_access_token == NULL)
    {
        ESP_LOGE(TAG, "Memory allocation failed");
        goto _exit;
    }
    memset(sg_access_token, '\0', token_len);
    snprintf(sg_access_token, "%s", token);
    token_updated_flag = true;
    ESP_LOGI(TAG, "Baidu access token = %s", sg_access_token);

_exit:
    free(url);
    esp_http_client_close(http_client);
    esp_http_client_cleanup(http_client);
    if (token)
        free(token);
}

/// @brief 获取 access token
/// @param  
/// @return 
char *baidu_get_access_token(void)
{
    return sg_access_token;
}

/// @brief 获取唯一的 cuid
/// @param  
/// @return 
char *baidu_get_cuid_by_mac(void)
{
    return sg_mac_str;
}
