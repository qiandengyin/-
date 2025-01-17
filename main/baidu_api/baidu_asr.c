#include "esp_log.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "json_utils.h"

#include "baidu_api.h"


static char *TAG = "BaiduAsr";

static char response_data[4096];
static int  recived_len;

// http客户端的事件处理回调函数
static esp_err_t http_client_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "connected to web-server");
        recived_len = 0;
        break;
    case HTTP_EVENT_ON_DATA:
        if (evt->user_data)
        {
            memcpy(evt->user_data + recived_len, evt->data, evt->data_len); // 将分片的每一片数据都复制到user_data
            recived_len += evt->data_len;                                   // 累计偏移更新
        }
        printf("HTTP_EVENT_ON_DATA, len=%d\n", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "finished a request and response!");
        recived_len = 0;
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "disconnected to web-server");
        recived_len = 0;
        break;
    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG, "error");
        recived_len = 0;
        break;
    default:
        break;
    }

    return ESP_OK;
}

/// @brief 语音转文字
/// @param audio_data 
/// @param audio_len 
/// @return 
char *baidu_get_asr_result(uint8_t *audio_data, int audio_len)
{
    char *asr_data = NULL;
    char url[256];
    char dev_pid[] = "1537";   // 普通话识别
    char *cuid = baidu_get_cuid_by_mac();
    char *access_token = baidu_get_access_token();
    if (access_token == NULL)
    {
        ESP_LOGE(TAG, "access token is NULL");
        return NULL;
    }
    
    sprintf(url, "http://vop.baidu.com/server_api?dev_pid=%s&cuid=%s&token=%s", dev_pid, cuid, access_token);

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_client_event_handler,
        .user_data = response_data,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "audio/wav;rate=16000");
    esp_http_client_set_post_field(client, (const char *)audio_data, audio_len);
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        cJSON *json = cJSON_Parse(response_data);
        if (json != NULL)
        {
            cJSON *result_json = cJSON_GetObjectItem(json, "result");
            if (result_json != NULL && cJSON_IsArray(result_json))
            {
                cJSON *result_array = cJSON_GetArrayItem(result_json, 0);
                if (result_array != NULL && cJSON_IsString(result_array))
                {
                    asr_data = strdup(result_array->valuestring);
                }
            }
            cJSON_Delete(json);
        }
    }
    else
    {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);

    return asr_data;
}
