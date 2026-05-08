/* ================================================================
 * mini_calendar.h - Mini Calendar widget for Digital Calendar Pi
 *
 * Phase 2.2.1: Skeleton with hardcoded data
 *   - 7x6 grid showing days of month
 *   - Today highlight (cyan filled circle)
 *   - Weekend coloring (Sun/Sat = pink)
 *   - Thai DOW headers (อา จ อ พ พฤ ศ ส)
 *   - Thai month + Buddhist year title
 *   - Nav buttons (visual only)
 *
 * Future:
 *   - 2.2.2: Real Thai calendar logic (lunar, holidays)
 *   - 2.2.3: Real time + auto-refresh
 *   - 2.2.4: Tulip background + translucent
 * ================================================================ */
#pragma once

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Build mini calendar widget on parent.
 * Uses hardcoded data: May 2569 BE (2026 CE), today = 8.
 *
 * Position: provide x, y. Size auto-calculated from layout tokens.
 */
void mini_calendar_build(lv_obj_t *parent, int x, int y);

#ifdef __cplusplus
}
#endif
