/* ================================================================
 * weather.cpp - Weather card (Phase 2.2.7 visual stub)
 *
 * Layout (480x300 by default):
 *   ┌──────────────────────────────────────┐
 *   │ กรุงเทพฯ                  (stub)      │  city + dev marker
 *   │                                       │
 *   │              32 °C                    │  big temperature
 *   │                                       │
 *   │ รู้สึกเหมือน 35 • ความชื้น 65%          │  meta line
 *   └──────────────────────────────────────┘
 *
 * All values are static. Phase 2.2.7-NET will:
 *   - add libcurl + cJSON to CMake
 *   - fetch https://api.open-meteo.com/...current_weather
 *   - refresh on a 5-minute lv_timer in a worker pthread
 *   - retain the same weather_build() signature so theme.cpp doesn't
 *     change
 * ================================================================ */
#include "weather.h"
#include "theme.h"
#include "../assets/thai_fonts.h"
#include "../assets/thai_strings.h"

#include <cstdio>

extern "C" void weather_build(lv_obj_t *parent, int x, int y, int w, int h)
{
    /* Card */
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_style_bg_color(card, C_BG_SECONDARY, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, 170, LV_PART_MAIN);    /* Phase 2.2.5h: more wallpaper bleed-through */
    lv_obj_set_style_border_color(card, C_BORDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 16, LV_PART_MAIN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* City title (top-left) */
    lv_obj_t *city = lv_label_create(card);
    lv_label_set_text(city, THAI_BANGKOK);            /* "กรุงเทพฯ" */
    lv_obj_set_style_text_color(city, C_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(city, &thai_sarabun_stacked_24, LV_PART_MAIN);
    lv_obj_align(city, LV_ALIGN_TOP_LEFT, 0, 0);

    /* (Phase 2.2.5l: removed the "(stub - Phase 2.2.7-NET)" dev marker
     * from the top-right. The stub status is captured in the commit
     * history / PROJECT_CONTEXT.md instead of cluttering the UI.) */

    /* Temperature value (centre, large) — Montserrat handles digits well */
    lv_obj_t *temp_value = lv_label_create(card);
    lv_label_set_text(temp_value, "32");
    lv_obj_set_style_text_color(temp_value, C_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_text_font(temp_value, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_transform_scale_x(temp_value, 410, LV_PART_MAIN);  /* 1.6x */
    lv_obj_set_style_transform_scale_y(temp_value, 410, LV_PART_MAIN);
    lv_obj_align(temp_value, LV_ALIGN_CENTER, -30, 0);

    /* Unit "°C" — must be a font with U+00B0 (degree). thai_sarabun was
     * built with range 32-127 + 3584-3711, so ° rendered as a tofu box.
     * Montserrat builtin includes Latin-1 Supplement (160-255), so ° works. */
    lv_obj_t *temp_unit = lv_label_create(card);
    lv_label_set_text(temp_unit, "°C");
    lv_obj_set_style_text_color(temp_unit, C_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_text_font(temp_unit, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align_to(temp_unit, temp_value, LV_ALIGN_OUT_RIGHT_MID, 16, -10);

    /* Meta line (bottom) — feels-like + humidity.
     * "รู้สึกเหมือน" in Thai font + "35°C" in Montserrat (Thai font has
     * no degree glyph, same reason as the temp_unit above). Two separate
     * labels stitched with lv_obj_align_to. */
    lv_obj_t *feels_th = lv_label_create(card);
    char fbuf[48];
    std::snprintf(fbuf, sizeof(fbuf), "%s", THAI_FEELS_LIKE);
    lv_label_set_text(feels_th, fbuf);
    lv_obj_set_style_text_color(feels_th, C_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_set_style_text_font(feels_th, &thai_sarabun_stacked_24, LV_PART_MAIN);
    lv_obj_align(feels_th, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    lv_obj_t *feels_val = lv_label_create(card);
    lv_label_set_text(feels_val, " 35°C");
    lv_obj_set_style_text_color(feels_val, C_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_set_style_text_font(feels_val, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align_to(feels_val, feels_th, LV_ALIGN_OUT_RIGHT_BOTTOM, 0, 0);

    lv_obj_t *humid_label = lv_label_create(card);
    lv_label_set_text(humid_label, "ความชื้น 65%");
    lv_obj_set_style_text_color(humid_label, C_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_set_style_text_font(humid_label, &thai_sarabun_stacked_24, LV_PART_MAIN);
    lv_obj_align(humid_label, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
}
