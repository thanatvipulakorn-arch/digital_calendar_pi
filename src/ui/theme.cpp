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

/* ──────────── Foundation UI (Phase 2.2.1) ──────────── */
#include "layout.h"
#include "mini_calendar.h"

extern "C" void build_foundation_ui(void)
{
    lv_obj_t *scr = lv_screen_active();

    /* Apply theme background */
    lv_obj_set_style_bg_color(scr, C_BG_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    /* Title (top center) */
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Digital Calendar - Pi");
    lv_obj_set_style_text_color(title, C_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_36, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

    /* Subtitle */
    lv_obj_t *subtitle = lv_label_create(scr);
    lv_label_set_text(subtitle, "Phase 2.2.2 - Thai Font");
    lv_obj_set_style_text_color(subtitle, C_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 60);

    /* Mini Calendar (centered horizontally, below title) */
    /* Card is 700x500, screen is 1280x720
     * X offset: (1280 - 700) / 2 = 290
     * Y offset: ~110 (below title+subtitle) */
    mini_calendar_build(scr, 290, 110);

    /* Footer hint (bottom) */
    lv_obj_t *footer = lv_label_create(scr);
    lv_label_set_text(footer, "Press Ctrl+C to exit");
    lv_obj_set_style_text_color(footer, C_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_set_style_text_font(footer, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -16);
}
