#include <string.h>
#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "audio_player.h"
#include "esp_crt_bundle.h"
#include "inttypes.h"

#include "app_audio.h"
#include "app_wifi.h"
#include "baidu_api.h"

static const char *TAG = "BaiduTts";

static uint32_t file_total_len = 0;

/* Define a function to handle HTTP events during an HTTP request */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
        file_total_len = 0;
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=(%" PRIu32 " + %d) [%d]", file_total_len, evt->data_len, MAX_FILE_SIZE);
        if ((file_total_len + evt->data_len) < MAX_FILE_SIZE)
        {
            memcpy(audio_rx_buffer + file_total_len, (char *)evt->data, evt->data_len);
            file_total_len += evt->data_len;
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH: %" PRIu32 ", %" PRIu32 " K", file_total_len, file_total_len / 1024);
        // 接收完成, 开始播放
        FILE *fp = NULL;
        fp = fmemopen((void *)audio_rx_buffer, file_total_len, "rb");
        if (fp)
            audio_player_play(fp);
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}

esp_err_t baidu_get_tts_result(char *audio_data, int audio_len)
{
    char *cuid = baidu_get_cuid_by_mac();
    char *access_token = baidu_get_access_token();
    if (access_token == NULL)
    {
        ESP_LOGE(TAG, "access token is NULL");
        return ESP_FAIL;
    }

    int body_size = snprintf(NULL, 0, "tex=&tok=%s&cuid=%s&ctp=1&lan=zh&spd=5&pit=5&vol=5&per=1&aue=3",
                             access_token,
                             cuid);
    body_size += audio_len;
    char *body = heap_caps_malloc((body_size + 1), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (body == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for body");
        return ESP_ERR_NO_MEM;
    }

    snprintf(body, body_size + 1, "tex=%s&tok=%s&cuid=%s&ctp=1&lan=zh&spd=5&pit=5&vol=5&per=1&aue=3",
             audio_data,
             access_token,
             cuid);
    // ----------------------------------------------------------------------------------------------

    esp_http_client_config_t config = {
        .url            = "http://tsn.baidu.com/text2audio",
        .buffer_size    = 128000,
        .buffer_size_tx = 4000,
        .timeout_ms     = 40000,
        .event_handler  = http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    esp_http_client_set_header(client, "Accept", "*/*");
    esp_http_client_set_post_field(client, (const char *)body, body_size);
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGE(TAG, "HTTP POST request success");
    }
    else
    {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);

    return err;
}


void baidu_tts_test(void)
{
    while (app_wifi_connected_already() != WIFI_STATUS_CONNECTED_OK)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    char *text = "你好，欢迎使用百度在线语音合成服务";
    baidu_get_tts_result(text, strlen(text));
}