/* ================================================================
 * theme.cpp - Theme implementation for Digital Calendar Pi
 *
 * Ported from ESP32 ui_theme.cpp (Phase 2.1)
 * ================================================================ */

#include "theme.h"

/* Active theme — set by theme_init() */
theme_t g_theme;

/* ──────────── Dark Theme Palette ──────────── */
static const theme_t THEME_DARK = {
    /* Backgrounds */
    .bg_primary   = LV_COLOR_MAKE(0x0F, 0x27, 0x42),  /* Navy #0F2742 */
    .bg_secondary = LV_COLOR_MAKE(0x1A, 0x3A, 0x5C),  /* Card navy */
    .bg_elevated  = LV_COLOR_MAKE(0x2A, 0x4A, 0x6F),  /* Hover navy */

    /* Accents */
    .accent       = LV_COLOR_MAKE(0x00, 0xC8, 0xE0),  /* Cyan #00C8E0 */
    .success      = LV_COLOR_MAKE(0x4C, 0xAF, 0x50),  /* Green */
    .warning      = LV_COLOR_MAKE(0xFF, 0xC1, 0x07),  /* Amber */
    .error        = LV_COLOR_MAKE(0xF4, 0x43, 0x36),  /* Red */

    /* Text */
    .text_primary = LV_COLOR_MAKE(0xFF, 0xFF, 0xFF),  /* White */
    .text_muted   = LV_COLOR_MAKE(0xB0, 0xC4, 0xDE),  /* Light blue-gray */
    .text_hint    = LV_COLOR_MAKE(0x70, 0x80, 0x90),  /* Slate gray */

    /* Borders */
    .border       = LV_COLOR_MAKE(0x2A, 0x4A, 0x6F),  /* Same as elevated */
};

/* ──────────── Light Theme Palette (placeholder, not used yet) ──────────── */
static const theme_t THEME_LIGHT = {
    .bg_primary   = LV_COLOR_MAKE(0xF5, 0xF5, 0xF5),
    .bg_secondary = LV_COLOR_MAKE(0xFF, 0xFF, 0xFF),
    .bg_elevated  = LV_COLOR_MAKE(0xE0, 0xE0, 0xE0),

    .accent       = LV_COLOR_MAKE(0x00, 0x7A, 0xCC),
    .success      = LV_COLOR_MAKE(0x2E, 0x7D, 0x32),
    .warning      = LV_COLOR_MAKE(0xF5, 0x7C, 0x00),
    .error        = LV_COLOR_MAKE(0xC6, 0x28, 0x28),

    .text_primary = LV_COLOR_MAKE(0x21, 0x21, 0x21),
    .text_muted   = LV_COLOR_MAKE(0x60, 0x60, 0x60),
    .text_hint    = LV_COLOR_MAKE(0x9E, 0x9E, 0x9E),

    .border       = LV_COLOR_MAKE(0xBD, 0xBD, 0xBD),
};

/* ──────────── API ──────────── */
extern "C" void theme_init(theme_mode_t mode)
{
    if (mode == THEME_MODE_LIGHT) {
        g_theme = THEME_LIGHT;
    } else {
        g_theme = THEME_DARK;  /* Default */
    }
}

/* ──────────── Foundation UI (Phase 2.2.1, expanded through 2.2.9) ──────────── */
#include "layout.h"
#include "mini_calendar.h"
#include "header.h"
#include "upcoming.h"
#include "timer_card.h"
#include "../assets/bg_tulip.h"

/* Native screen size — must match boards/<board>.cmake / window settings. */
#define SCREEN_W   1280
#define SCREEN_H   720

/* Layout — 1280x720 with bg_tulip behind everything.
 *
 *  ┌──────────────────────────────────────────────────────────┐
 *  │  Header (1280x100): weekday/date/lunar | clock | wanphra │
 *  ├───────────────────────────────────┬──────────────────────┤
 *  │                                   │  Upcoming holidays   │
 *  │  Mini Calendar 700x500            │  (380x340)           │
 *  │  centred under header             ├──────────────────────┤
 *  │                                   │  Timer card          │
 *  │                                   │  (380x140)           │
 *  └───────────────────────────────────┴──────────────────────┘
 *  Subtitle ("Phase 2.2.x") tucked bottom-left as a faint dev marker.
 */
#define HDR_H        100
#define MINI_X       60
#define MINI_Y       (HDR_H + 16)
#define UPCOMING_X   880
#define UPCOMING_Y   (HDR_H + 16)
#define UPCOMING_W   340
#define UPCOMING_H   340
#define TIMER_X      880
#define TIMER_Y      (UPCOMING_Y + UPCOMING_H + 16)
#define TIMER_W      340
#define TIMER_H      140

extern "C" void build_foundation_ui(void)
{
    lv_obj_t *scr = lv_screen_active();

    /* ── Tulip background (z-index 0) ──
     * Stretched to full screen via STRETCH inner-align — LVGL 9 idiom.
     * (Earlier scale_x/scale_y attempt rendered with the right pixel size
     * but kept the widget bbox at 800x480, so the stretched pixels were
     * clipped to the original area. STRETCH grows the bbox AND interp.) */
    lv_obj_t *bg = lv_image_create(scr);
    lv_image_set_src(bg, &bg_tulip);
    lv_obj_set_pos(bg, 0, 0);
    lv_obj_set_size(bg, SCREEN_W, SCREEN_H);
    lv_image_set_inner_align(bg, LV_IMAGE_ALIGN_STRETCH);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);

    /* ── Header bar (top) ── */
    header_build(scr, 0, 0, SCREEN_W, HDR_H);

    /* ── Mini calendar — left of column 3 ── */
    mini_calendar_build(scr, MINI_X, MINI_Y);

    /* ── Upcoming holidays card (right column, top) ── */
    upcoming_build(scr, UPCOMING_X, UPCOMING_Y, UPCOMING_W, UPCOMING_H);

    /* ── Timer card (right column, bottom) ── */
    timer_card_build(scr, TIMER_X, TIMER_Y, TIMER_W, TIMER_H);

    /* ── Phase marker (bottom-left, faint) ── */
    lv_obj_t *subtitle = lv_label_create(scr);
    lv_label_set_text(subtitle, "Phase 2.2.9 - Header / Upcoming / Timer");
    lv_obj_set_style_text_color(subtitle, C_TEXT_HINT, LV_PART_MAIN);
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(subtitle, LV_ALIGN_BOTTOM_LEFT, 12, -8);
}
