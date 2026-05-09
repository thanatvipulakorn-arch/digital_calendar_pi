/* ================================================================
 * timer_card.cpp - Kitchen-timer card (Phase 2.2.9 — visual placeholder)
 *
 * Phase 2.2.9 scope (this file):
 *   - Card visual matches the rest of the UI (translucent, cyan border)
 *   - Bell glyph + "Timer" title on left
 *   - "Tap to set" hint on right (no click handler yet)
 *
 * Deferred to a later phase (need GPIO/audio HAT decision):
 *   - Picker popup (+/− minutes) — port from ESP32 timer_popup_open()
 *   - Countdown state machine (IDLE/RUNNING/FINISHED)
 *   - Buzzer drive
 * ================================================================ */
#include "timer_card.h"
#include "theme.h"

extern "C" void timer_card_build(lv_obj_t *parent, int x, int y, int w, int h)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_style_bg_color(card, C_BG_SECONDARY, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, 220, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, C_BORDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 16, LV_PART_MAIN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Left: bell + Timer */
    lv_obj_t *lbl_l = lv_label_create(card);
    lv_label_set_text(lbl_l, LV_SYMBOL_BELL "  Timer");
    lv_obj_set_style_text_color(lbl_l, C_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_l, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(lbl_l, LV_ALIGN_LEFT_MID, 0, 0);

    /* Right: hint */
    lv_obj_t *lbl_r = lv_label_create(card);
    lv_label_set_text(lbl_r, "Tap to set");
    lv_obj_set_style_text_color(lbl_r, C_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_r, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(lbl_r, LV_ALIGN_RIGHT_MID, 0, 0);
}
