/* ================================================================
 * upcoming.h - Upcoming holidays card (Phase 2.2.8)
 *
 * Scans the next 90 days for holidays (lunar Buddhist + Gregorian-fixed)
 * via thai_calendar_*, sorts ascending by date offset, and shows up to 5.
 * ================================================================ */
#pragma once

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void upcoming_build(lv_obj_t *parent, int x, int y, int w, int h);

#ifdef __cplusplus
}
#endif
