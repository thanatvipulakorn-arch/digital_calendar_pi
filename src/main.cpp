/* ================================================================
 * main.cpp - Digital Calendar Pi entry point
 *
 * Phase 2.1: Foundation
 *   - Initialize LVGL + framebuffer
 *   - Initialize theme (Dark)
 *   - Show theme background + version label
 *
 * Future:
 *   - Phase 2.2: Mini calendar UI
 *   - Phase 2.3: Thai calendar logic
 *   - Phase 2.4: Real time
 *   - Phase 2.5: Background image + theme switching
 * ================================================================ */

#include "lvgl/lvgl.h"
#include "lvgl/src/drivers/display/fb/lv_linux_fbdev.h"
#include "ui/theme.h"
#include "ui/layout.h"

#include <unistd.h>
#include <stdio.h>
#include <time.h>

/* ──────────── LVGL tick callback (Linux monotonic clock) ──────────── */
static uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
    if (start_ms == 0) {
        struct timespec tv_start;
        clock_gettime(CLOCK_MONOTONIC, &tv_start);
        start_ms = ((uint64_t)tv_start.tv_sec * 1000) + (tv_start.tv_nsec / 1000000);
    }
    struct timespec tv_now;
    clock_gettime(CLOCK_MONOTONIC, &tv_now);
    uint64_t now_ms = ((uint64_t)tv_now.tv_sec * 1000) + (tv_now.tv_nsec / 1000000);
    return (uint32_t)(now_ms - start_ms);
}

/* ──────────── Build foundation UI ──────────── */
static void build_foundation_ui(void)
{
    lv_obj_t *scr = lv_screen_active();

    /* Apply theme background */
    lv_obj_set_style_bg_color(scr, C_BG_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    /* Title label - top center */
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Digital Calendar — Pi");
    lv_obj_set_style_text_color(title, C_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, PADDING * 2);

    /* Version subtitle */
    lv_obj_t *subtitle = lv_label_create(scr);
    lv_label_set_text(subtitle, "Phase 2.1 — Foundation");
    lv_obj_set_style_text_color(subtitle, C_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, PADDING * 2 + 60);

    /* Sample card (translucent placeholder) - center */
    lv_obj_t *card = lv_obj_create(scr);
    lv_obj_set_size(card, 600, 300);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_style_bg_color(card, C_BG_SECONDARY, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, C_BORDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(card, CARD_RADIUS, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, PADDING, LV_PART_MAIN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Card content - status text */
    lv_obj_t *card_text = lv_label_create(card);
    lv_label_set_text(card_text,
        "Theme:    Dark (navy palette)\n"
        "Layout:   1280 x 720 (HDMI native)\n"
        "Status:   Foundation ready\n"
        "\n"
        "Next: Phase 2.2 — Calendar UI");
    lv_obj_set_style_text_color(card_text, C_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(card_text, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_line_space(card_text, 8, LV_PART_MAIN);
    lv_obj_center(card_text);

    /* Footer hint */
    lv_obj_t *footer = lv_label_create(scr);
    lv_label_set_text(footer, "Press Ctrl+C to exit");
    lv_obj_set_style_text_color(footer, C_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_set_style_text_font(footer, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -PADDING * 2);
}

/* ──────────── Entry point ──────────── */
int main(void)
{
    /* 1. Initialize LVGL */
    lv_init();
    lv_tick_set_cb(custom_tick_get);

    /* 2. Initialize framebuffer display */
    lv_display_t *disp = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(disp, "/dev/fb0");

    /* 3. Initialize theme (BEFORE building UI) */
    theme_init(THEME_MODE_DARK);

    /* 4. Build UI */
    build_foundation_ui();

    /* 5. Main loop */
    while (1) {
        uint32_t time_till_next = lv_timer_handler();
        usleep(time_till_next * 1000);
    }

    return 0;
}
