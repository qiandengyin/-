/* Copyright 2018 James Laird-Wah
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "rgb_matrix_drivers.h"

#include <stdbool.h>
#include "led_strip.h"
#if defined(RGB_MATRIX_ENABLE) && defined(RGB_MATRIX_SPLIT)
#include "keyboard.h"
#endif
#include "color.h"
// #include "util.h"

/* Each driver needs to define the struct
 *    const rgb_matrix_driver_t rgb_matrix_driver;
 * All members must be provided.
 * Keyboard custom drivers can define this in their own files, it should only
 * be here if shared between boards.
 */
#    if defined(RGBLIGHT_WS2812)
#        pragma message "Cannot use RGBLIGHT and RGB Matrix using WS2812 at the same time."
#        pragma message "You need to use a custom driver, or re-implement the WS2812 driver to use a different configuration."
#    endif

// LED color buffer
static led_strip_handle_t led_strip = NULL;
static uint32_t led_count = 0;
rgb_led_t *rgb_matrix_ws2812_array = NULL;
bool      ws2812_dirty = false;

static void init(void)
{
    ws2812_dirty = false;
}

static void flush(void)
{
    if (ws2812_dirty) {
        led_strip_refresh(led_strip);
        ws2812_dirty = false;
    }
}

// Set an led in the buffer to a color
static inline void setled(int i, uint8_t r, uint8_t g, uint8_t b)
{
#    if defined(RGB_MATRIX_ENABLE) && defined(RGB_MATRIX_SPLIT)
    const uint8_t k_rgb_matrix_split[2] = RGB_MATRIX_SPLIT;
    if (!is_keyboard_left()) {
        if (i >= k_rgb_matrix_split[0]) {
            i -= k_rgb_matrix_split[0];
        } else {
            return;
        }
    } else if (i >= k_rgb_matrix_split[0]) {
        return;
    }
#    endif

    ws2812_dirty                 = true;
    led_strip_set_pixel(led_strip, i, r, g, b);

#    ifdef RGBW
    convert_rgb_to_rgbw(&rgb_matrix_ws2812_array[i]);
#    endif
}

static void setled_all(uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < led_count; i++) {
        setled(i, r, g, b);
    }
}

const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = init,
    .flush         = flush,
    .set_color     = setled,
    .set_color_all = setled_all,
};

void rgb_matrix_driver_init(led_strip_handle_t handle, uint32_t strip_num)
{
    led_strip = handle;
    led_count = strip_num;
    rgb_matrix_ws2812_array = (rgb_led_t *)malloc(strip_num * sizeof(rgb_led_t));
}
