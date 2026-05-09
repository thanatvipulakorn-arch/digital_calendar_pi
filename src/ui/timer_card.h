/* ================================================================
 * timer_card.h - Kitchen-timer card (Phase 2.2.9 — visual placeholder)
 *
 * Phase 2.2.9 scope: card visual + "Tap to set" idle text. Picker popup
 * + countdown state machine + buzzer are deferred (need GPIO/audio HAT
 * — explicitly out of scope per current decision).
 * ================================================================ */
#pragma once

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void timer_card_build(lv_obj_t *parent, int x, int y, int w, int h);

#ifdef __cplusplus
}
#endif
