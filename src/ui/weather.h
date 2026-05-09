/* ================================================================
 * weather.h - Weather card (Phase 2.2.7 stub)
 *
 * Phase 2.2.5d scope: visual placeholder only — fixed text values, no
 * network. Phase 2.2.7-NET (next batch) will wire libcurl + cJSON to
 * Open-Meteo and refresh on a 5-minute lv_timer.
 * ================================================================ */
#pragma once

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void weather_build(lv_obj_t *parent, int x, int y, int w, int h);

#ifdef __cplusplus
}
#endif
