/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"

#include "audio_player.h"
#include "file_iterator.h"
#include "bsp_keyboard.h"
#include "app_sr.h"
#include "app_audio.h"
#include "app_wifi.h"
#include "chatgpt_api.h"
#include "baidu_api.h"
#include "keyboard.h"

static const char *TAG = "app_audio";

#define CONFIG_VOLUME_LEVEL 90

bool record_flag = false;
uint32_t record_total_len = 0;
uint32_t file_total_len = 0;
static uint8_t *record_audio_buffer = NULL;
uint8_t *audio_rx_buffer = NULL;
audio_play_finish_cb_t audio_play_finish_cb = NULL;

extern sr_data_t *g_sr_data;
extern int Cache_WriteBack_Addr(uint32_t addr, uint32_t size);

static esp_err_t audio_mute_function(AUDIO_PLAYER_MUTE_SETTING setting)
{
    bsp_codec_mute_set(setting == AUDIO_PLAYER_MUTE ? true : false);
    // restore the voice volume upon unmuting
    if (setting == AUDIO_PLAYER_UNMUTE)
    {
        bsp_codec_volume_set(CONFIG_VOLUME_LEVEL, NULL);
    }
    return ESP_OK;
}

static esp_err_t audio_codec_set_fs(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    esp_err_t ret = ESP_OK;
    ret = bsp_codec_set_fs(rate, bits_cfg, ch);

    bsp_codec_mute_set(true);
    bsp_codec_mute_set(false);
    bsp_codec_volume_set(CONFIG_VOLUME_LEVEL, NULL);
    vTaskDelay(pdMS_TO_TICKS(50));

    return ret;
}

static void audio_player_cb(audio_player_cb_ctx_t *ctx)
{
    switch (ctx->audio_event)
    {
    case AUDIO_PLAYER_CALLBACK_EVENT_IDLE:
        ESP_LOGI(TAG, "Player IDLE");
        bsp_codec_set_fs(16000, 16, 2);
        if (audio_play_finish_cb)
        {
            audio_play_finish_cb();
        }
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_COMPLETED_PLAYING_NEXT:
        ESP_LOGI(TAG, "Player NEXT");
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_PLAYING:
        ESP_LOGI(TAG, "Player PLAYING");
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_PAUSE:
        ESP_LOGI(TAG, "Player PAUSE");
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_SHUTDOWN:
        ESP_LOGI(TAG, "Player SHUTDOWN");
        break;
    default:
        break;
    }
}

void audio_record_init()
{
#if DEBUG_SAVE_PCM
    // 分配录音缓存
    record_audio_buffer = heap_caps_calloc(1, RECORD_FILE_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    assert(record_audio_buffer);
    printf("successfully created record_audio_buffer with a size: %zu\r\n", RECORD_FILE_SIZE);
    // 分配语音合成缓存
    audio_rx_buffer = heap_caps_calloc(1, MAX_FILE_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    assert(audio_rx_buffer);
    printf("successfully created audio_rx_buffer with a size: %zu\r\n", MAX_FILE_SIZE);
#endif

    if (record_audio_buffer == NULL || audio_rx_buffer == NULL)
    {
        printf("Error: Failed to allocate memory for buffers\r\n");
        return;
    }

    file_iterator_instance_t *file_iterator = file_iterator_new(BSP_SPIFFS_MOUNT_POINT);
    assert(file_iterator != NULL);

    audio_player_config_t config = {
        .mute_fn = audio_mute_function,
        .write_fn = bsp_i2s_write,
        .clk_set_fn = audio_codec_set_fs,
        .priority = 5,
    };
    ESP_ERROR_CHECK(audio_player_new(config));
    audio_player_callback_register(audio_player_cb, NULL);
    ESP_LOGI(TAG, "audio_record_init -----> Audio player initialized");
    
    // 开机音效
    FILE *fp = fopen("/spiffs/powerOn.wav", "r");
    if (fp)
    {
        audio_player_play(fp);
    }
}

void audio_record_save(int16_t *audio_buffer, int audio_chunksize)
{
#if DEBUG_SAVE_PCM
    if (record_flag)
    {
        uint16_t *record_buff = (uint16_t *)(record_audio_buffer + sizeof(wav_header_t));
        record_buff += record_total_len;
        for (int i = 0; i < (audio_chunksize - 1); i++)
        {
            if (record_total_len < (MAX_FILE_SIZE - sizeof(wav_header_t)) / 2)
            {
#if PCM_ONE_CHANNEL
                record_buff[i * 1 + 0] = audio_buffer[i * 3 + 0];
                record_total_len += 1;
#else
                record_buff[i * 2 + 0] = audio_buffer[i * 3 + 0];
                record_buff[i * 2 + 1] = audio_buffer[i * 3 + 1];
                record_total_len += 2;
#endif
            }
        }
    }
#endif
}

void audio_register_play_finish_cb(audio_play_finish_cb_t cb)
{
    audio_play_finish_cb = cb;
}

static void audio_record_start()
{
#if DEBUG_SAVE_PCM
    ESP_LOGI(TAG, "### record Start");
    audio_player_stop();
    record_flag = true;
    record_total_len = 0;
    file_total_len = sizeof(wav_header_t);
#endif
}

static esp_err_t audio_record_stop()
{
    esp_err_t ret = ESP_OK;
#if DEBUG_SAVE_PCM
    record_flag = false;
#if PCM_ONE_CHANNEL
    record_total_len *= 1;
#else
    record_total_len *= 2;
#endif
    file_total_len += record_total_len;
    ESP_LOGI(TAG, "### record Stop, %" PRIu32 " %" PRIu32 "K",
             record_total_len,
             record_total_len / 1024);

    FILE *fp = fopen("/spiffs/echo_en_wake.wav", "r");
    ESP_GOTO_ON_FALSE(NULL != fp, ESP_FAIL, err, TAG, "Failed create record file");

    wav_header_t wav_head;
    int len = fread(&wav_head, 1, sizeof(wav_header_t), fp);
    ESP_GOTO_ON_FALSE(len > 0, ESP_FAIL, err, TAG, "Failed create record file");

    wav_head.SampleRate = 16000;
#if PCM_ONE_CHANNEL
    wav_head.NumChannels = 1;
#else
    wav_head.NumChannels = 2;
#endif
    wav_head.BitsPerSample = 16;
    wav_head.ChunkSize = file_total_len - 8;
    wav_head.ByteRate  = wav_head.SampleRate * wav_head.BitsPerSample * wav_head.NumChannels / 8;
    wav_head.Subchunk2ID[0] = 'd';
    wav_head.Subchunk2ID[1] = 'a';
    wav_head.Subchunk2ID[2] = 't';
    wav_head.Subchunk2ID[3] = 'a';
    wav_head.Subchunk2Size = record_total_len;
    memcpy((void *)record_audio_buffer, &wav_head, sizeof(wav_header_t));
    Cache_WriteBack_Addr((uint32_t)record_audio_buffer, record_total_len);
    // 播放录音
    // FILE *fp1 = NULL;
    // fp1 = fmemopen((void *)record_audio_buffer, record_total_len, "rb");
    // if (fp1)
    //     audio_player_play(fp1);
#endif
err:
    if (fp)
    {
        fclose(fp);
    }
    return ret;
}

esp_err_t audio_play_task(void *filepath)
{
    FILE *fp = NULL;
    struct stat file_stat;
    esp_err_t ret = ESP_OK;

    const size_t chunk_size = 4096;
    uint8_t *buffer = malloc(chunk_size);
    ESP_GOTO_ON_FALSE(NULL != buffer, ESP_FAIL, EXIT, TAG, "buffer malloc failed");

    ESP_GOTO_ON_FALSE(-1 != stat(filepath, &file_stat), ESP_FAIL, EXIT, TAG, "Failed to stat file");

    fp = fopen(filepath, "r");
    ESP_GOTO_ON_FALSE(NULL != fp, ESP_FAIL, EXIT, TAG, "Failed create record file");

    wav_header_t wav_head;
    int len = fread(&wav_head, 1, sizeof(wav_header_t), fp);
    ESP_GOTO_ON_FALSE(len > 0, ESP_FAIL, EXIT, TAG, "Read wav header failed");

    if (NULL == strstr((char *)wav_head.Subchunk1ID, "fmt") &&
        NULL == strstr((char *)wav_head.Subchunk2ID, "data"))
    {
        ESP_LOGI(TAG, "PCM format");
        fseek(fp, 0, SEEK_SET);
        wav_head.SampleRate = 16000;
        wav_head.NumChannels = 2;
        wav_head.BitsPerSample = 16;
    }

    ESP_LOGI(TAG, "frame_rate= %" PRIi32 ", ch=%d, width=%d", wav_head.SampleRate, wav_head.NumChannels, wav_head.BitsPerSample);
    bsp_codec_set_fs(wav_head.SampleRate, wav_head.BitsPerSample, I2S_SLOT_MODE_STEREO);

    bsp_codec_mute_set(true);
    bsp_codec_mute_set(false);
    bsp_codec_volume_set(CONFIG_VOLUME_LEVEL, NULL);

    size_t cnt, total_cnt = 0;
    do
    {
        /* Read file in chunks into the scratch buffer */
        len = fread(buffer, 1, chunk_size, fp);
        if (len <= 0)
        {
            break;
        }
        else if (len > 0)
        {
            bsp_i2s_write(buffer, len, &cnt, portMAX_DELAY);
            total_cnt += cnt;
        }
    } while (1);

EXIT:
    if (fp)
    {
        fclose(fp);
    }
    if (buffer)
    {
        free(buffer);
    }
    return ret;
}

void sr_handler_task(void *pvParam)
{
    ESP_LOGI(TAG, "sr_handler_task");

    while (true)
    {
        if (NEED_DELETE && xEventGroupGetBits(g_sr_data->event_group))
        {
            ESP_LOGI(TAG, "Delete sr_handler_task");
            xEventGroupSetBits(g_sr_data->event_group, HANDLE_DELETED);
            vTaskDelete(NULL);
        }

        sr_result_t result = {
            .wakenet_mode = WAKENET_NO_DETECT,
            .state = ESP_MN_STATE_DETECTING,
            .command_id = 0,
        };

        app_sr_get_result(&result, pdMS_TO_TICKS(1 * 1000));

        // 长时间未检测到命令词
        if (ESP_MN_STATE_TIMEOUT == result.state)
        {
            ESP_LOGI(TAG, "ESP_MN_STATE_TIMEOUT");
            audio_record_stop();// 停止录音
            FILE *fp = fopen("/spiffs/waitPlease.mp3", "r");
            if (fp)
            {
                audio_player_play(fp);
            }
            
            if (WIFI_STATUS_CONNECTED_OK == app_wifi_connected_already())
            {
                app_wifi_lock(0);
                chatgpt_start((uint8_t *)record_audio_buffer, record_total_len);
                app_wifi_unlock();
            }
            continue;
        }

        // 识别到唤醒词
        if (WAKENET_DETECTED == result.wakenet_mode)
        {
            audio_play_task("/spiffs/echo_cn_wake.wav");
            audio_record_start();// 开始录音
            continue;
        }

        // 识别到命令词
        if (ESP_MN_STATE_DETECTED & result.state)
        {
            ESP_LOGE(TAG, "STOP:%d", result.command_id);
            audio_record_stop();// 停止录音
            audio_play_task("/spiffs/echo_cn_ok.wav");
            if (WIFI_STATUS_CONNECTED_OK != app_wifi_connected_already())
                continue;
            switch (result.command_id)
            {
            case 0x55:
                // 语音转文字
                app_wifi_lock(0);
                char *recognition_result = baidu_get_asr_result((uint8_t *)record_audio_buffer, record_total_len);
                app_wifi_unlock();
                if (recognition_result == NULL)
                {
                    ESP_LOGE(TAG, "0. No text recognized");
                    continue;
                }

                if (strlen(recognition_result) == 0)
                {
                    ESP_LOGE(TAG, "1. No text recognized");
                    continue;
                }

                ESP_LOGE(TAG, "user input: %s", recognition_result);
                ESP_LOGE(TAG, "user input length: %d", strlen(recognition_result));
                // TODO: something
                break;
            default:
                break;
            }
        }
    }
    vTaskDelete(NULL);
}
