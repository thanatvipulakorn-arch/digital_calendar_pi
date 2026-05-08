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
 * Use for body text, calendar cells, headers. */
LV_FONT_DECLARE(thai_sarabun_24);

#ifdef __cplusplus
}
#endif
