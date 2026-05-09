/* ================================================================
 * header.h - Top header bar widget (Phase 2.2.6)
 *
 * Layout (1280 wide, 100 tall):
 *   ┌──────────────────────────────────────────────────────────┐
 *   │  วันเสาร์                          09:00:42  ●วันพระวันนี้ │
 *   │  9 พฤษภาคม 2569                    NTP: synced            │
 *   │  ขึ้น 8 ค่ำ เดือน 6 | ปีมะเมีย                              │
 *   └──────────────────────────────────────────────────────────┘
 *
 * Static labels are created in header_build(). The 1 Hz lv_timer set up
 * in main() calls header_tick() which refreshes the clock every second
 * and the date / lunar / weekday on midnight rollover (cheap).
 * ================================================================ */
#pragma once

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void header_build(lv_obj_t *parent, int x, int y, int w, int h);
void header_tick(void);   /* call from a 1 Hz lv_timer */

#ifdef __cplusplus
}
#endif
