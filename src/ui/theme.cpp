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
#include "../assets/bg_calendar.h"

/* Native screen size — Samsung 27" HDMI framebuffer is 1920x1080.
 * If you change this, also update the default window size in main.c
 * (configure_simulator). */
#undef SCREEN_W   /* layout.h re-defines for the ESP32 800x480 spec */
#undef SCREEN_H
#define SCREEN_W   1920
#define SCREEN_H   1080

/* Layout — 1920x1080 with bg_calendar (1920x1080 native) behind.
 * Generous EDGE_MARGIN (60 px) to survive Samsung TV overscan that
 * was clipping the upcoming card off the right edge in v1.
 *
 *  ┌──────────────────────────────────────────────────────────────────┐
 *  │  Header (1920x130): weekday/date/lunar | clock | wanphra         │
 *  ├──────────────────────────────────────────┬───────────────────────┤
 *  │                                          │  Upcoming holidays    │
 *  │       Mini Calendar 1100x720             │  (540x620)            │
 *  │       (bumped up from 700x500)           ├───────────────────────┤
 *  │                                          │  Timer card (540x200) │
 *  └──────────────────────────────────────────┴───────────────────────┘
 *  Subtitle ("Phase 2.2.x") tucked bottom-left as a faint dev marker.
 */
#undef HDR_H      /* layout.h had HDR_H=90 for the 800x480 build */
#define HDR_H            130
#define COL_GAP          32
#define EDGE_MARGIN      60   /* cushion against TV overscan */

/* Right column for upcoming + timer cards */
#define RIGHT_W          540
#define RIGHT_X          (SCREEN_W - RIGHT_W - EDGE_MARGIN)   /* = 1320 */

#define MINI_X           140    /* centre-ish in left zone (60..1320) */
#define MINI_Y           (HDR_H + COL_GAP)

#define UPCOMING_X       RIGHT_X
#define UPCOMING_Y       (HDR_H + COL_GAP)
#define UPCOMING_W       RIGHT_W
#define UPCOMING_H       620
#define TIMER_X          RIGHT_X
#define TIMER_Y          (UPCOMING_Y + UPCOMING_H + COL_GAP)
#define TIMER_W          RIGHT_W
#define TIMER_H          200

extern "C" void build_foundation_ui(void)
{
    lv_obj_t *scr = lv_screen_active();

    /* ── Calendar background (z-index 0) ──
     * bg_calendar is now 1920x1080 RGB565 (native HDMI res), so STRETCH
     * is effectively 1:1 — no upscale blur like the earlier 800x480
     * bg_tulip suffered from. */
    lv_obj_t *bg = lv_image_create(scr);
    lv_image_set_src(bg, &bg_calendar);
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
    lv_label_set_text(subtitle, "Phase 2.2.5b - 1920x1080 layout");
    lv_obj_set_style_text_color(subtitle, C_TEXT_HINT, LV_PART_MAIN);
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(subtitle, LV_ALIGN_BOTTOM_LEFT, EDGE_MARGIN, -16);
}
