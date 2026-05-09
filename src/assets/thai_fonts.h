/* ================================================================
 * thai_fonts.h - Thai font declarations
 *
 * Custom Thai fonts ported from ESP32 project.
 * Generated from Sarabun-Regular.ttf via LVGL font converter.
 *
 * Range: ASCII 32-127 + Thai 0x0E00-0x0E7F (3584-3711)
 * Format: 4 bpp (anti-aliased)
 * ================================================================ */
#pragma once

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Thai Sarabun font, 24px, 4bpp.
 * Regular variant — use for plain text without vowel/tone stacking. */
LV_FONT_DECLARE(thai_sarabun_24);

/* Stacked variant — tone marks raised +6 px to clear vowel-above stacks
 * like ขึ้น (ึ + ้), ที่ (ี + ่), นี้ (ี + ้), รู้ (ู + ้). Same ASCII +
 * Thai range as the regular variant; line_height = 38 (vs 34). Phase
 * 2.2.5d: swept across all UI labels for consistency. */
LV_FONT_DECLARE(thai_sarabun_stacked_24);

#ifdef __cplusplus
}
#endif
