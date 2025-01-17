#ifndef _BSP_AUDIO_H_
#define _BSP_AUDIO_H_

#include <stdint.h>
#include "esp_err.h"
#include "driver/i2s_std.h"

esp_err_t bsp_codec_mute_set(bool enable);
esp_err_t bsp_codec_volume_set(int volume, int *volume_set);
esp_err_t bsp_codec_set_fs(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch);
esp_err_t bsp_i2s_read(void *audio_buffer, size_t len, size_t *bytes_read, uint32_t timeout_ms);
esp_err_t bsp_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms);
esp_err_t bsp_i2c_init(void);
esp_err_t bsp_audio_init(void);

void bsp_audio_mute_enable(bool enable);
bool bsp_audio_mute_is_enable(void);

#endif /* _BSP_AUDIO_H_ */