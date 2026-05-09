/* ================================================================
 * bg_tulip.h
 *
 * Tulip background image (Phase 2.2.5).
 * Source: bg_tulip.c — 800x480 RGB565, ported verbatim from ESP32.
 * Pi displays it scaled to fill 1280x720 (slight 5:3 → 16:9 stretch).
 * ================================================================ */
#pragma once

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

LV_IMAGE_DECLARE(bg_tulip);

#ifdef __cplusplus
}
#endif
