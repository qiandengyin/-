#ifndef BAIDU_API_H
#define BAIDU_API_H

void baidu_update_access_token(void);
char *baidu_get_access_token(void);
char *baidu_get_cuid_by_mac(void);
char *baidu_get_asr_result(uint8_t *audio_data, int audio_len);
esp_err_t baidu_get_tts_result(char *audio_data, int audio_len);

#endif // BAIDU_API_H