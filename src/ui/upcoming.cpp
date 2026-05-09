/* ================================================================
 * upcoming.cpp - Upcoming holidays card (Phase 2.2.8)
 *
 * Strategy: snapshot at build time. Scan today..today+90 days, collect
 * (offset, month, day, name) tuples whenever thai_calendar_holiday_en()
 * or thai_calendar_fixed_holiday_en() returns non-NULL. Sort ascending
 * by offset, render up to UPCOMING_MAX rows.
 *
 * Auto-refresh on midnight is deferred to Phase 2.2.5+ (would require
 * day_changed signal — same channel as header_tick uses).
 * ================================================================ */
#include "upcoming.h"
#include "theme.h"
#include "../assets/thai_fonts.h"
#include "../assets/thai_strings.h"
#include "../calendar/thai_calendar.h"

#include <cstdio>
#include <ctime>

#define UPCOMING_MAX        5
#define UPCOMING_LOOKAHEAD  90    /* days */

typedef struct {
    int   offset;          /* days from today (0 = today, 1 = tomorrow, ...) */
    int   month;
    int   day;
    const char *name_en;   /* English name (we render Thai via lookup) */
} upcoming_item_t;

static int compare_offset(const upcoming_item_t *a, const upcoming_item_t *b)
{
    return a->offset - b->offset;
}

static void scan_and_collect(upcoming_item_t out[UPCOMING_MAX], int *count)
{
    *count = 0;
    time_t now = time(NULL);
    struct tm tm_today;
    localtime_r(&now, &tm_today);

    /* Bubble-collect first UPCOMING_MAX in offset order. We don't need a
     * full sort — keep the array sorted as we insert. */
    for (int n = 0; n < UPCOMING_LOOKAHEAD; n++) {
        time_t t = now + (time_t)n * 86400;
        struct tm tm_fut;
        localtime_r(&t, &tm_fut);
        int month = tm_fut.tm_mon + 1;
        int day   = tm_fut.tm_mday;

        const char *name = thai_calendar_fixed_holiday_en(month, day);
        if (!name) {
            thai_lunar_t lun = thai_calendar_get_lunar(t);
            if (lun.valid) {
                name = thai_calendar_holiday_en(&lun);
            }
        }
        if (!name) continue;

        if (*count < UPCOMING_MAX) {
            out[*count].offset  = n;
            out[*count].month   = month;
            out[*count].day     = day;
            out[*count].name_en = name;
            (*count)++;

            /* Insertion sort step (forward — items collected by ascending n,
             * so already sorted; this is a guard if logic changes later). */
            for (int i = *count - 1; i > 0; i--) {
                if (compare_offset(&out[i - 1], &out[i]) > 0) {
                    upcoming_item_t tmp = out[i - 1];
                    out[i - 1] = out[i];
                    out[i] = tmp;
                } else break;
            }
        } else {
            /* Already full and sorted; new offset is >= last (we scan
             * forward only) → discard. */
            break;
        }
    }
}

extern "C" void upcoming_build(lv_obj_t *parent, int x, int y, int w, int h)
{
    /* Card */
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

    /* Title */
    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, THAI_UPCOMING_HOL);
    lv_obj_set_style_text_color(title, C_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &thai_sarabun_stacked_24, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    /* Scan */
    upcoming_item_t items[UPCOMING_MAX];
    int count = 0;
    scan_and_collect(items, &count);

    if (count == 0) {
        lv_obj_t *lbl = lv_label_create(card);
        lv_label_set_text(lbl, "ไม่มีวันหยุดใน 90 วันข้างหน้า");
        lv_obj_set_style_text_color(lbl, C_TEXT_MUTED, LV_PART_MAIN);
        lv_obj_set_style_text_font(lbl, &thai_sarabun_stacked_24, LV_PART_MAIN);
        lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 0, 40);
        return;
    }

    /* Rows */
    const int row_h = 38;
    const int row_y0 = 44;

    for (int i = 0; i < count; i++) {
        int row_y = row_y0 + i * row_h;

        /* Date + Thai name */
        const char *th = thai_holiday_lookup(items[i].name_en);
        const char *th_or_en = th ? th : items[i].name_en;
        const char *mon_th = (items[i].month >= 1 && items[i].month <= 12)
                                ? THAI_MONTH_SHORT[items[i].month]
                                : "";
        char left[128];
        std::snprintf(left, sizeof(left), "%d %s  %s",
                      items[i].day, mon_th, th_or_en);

        lv_obj_t *l = lv_label_create(card);
        lv_label_set_text(l, left);
        lv_obj_set_style_text_color(l, C_TEXT_PRIMARY, LV_PART_MAIN);
        lv_obj_set_style_text_font(l, &thai_sarabun_stacked_24, LV_PART_MAIN);
        lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
        lv_obj_set_width(l, w - 90);
        lv_obj_set_pos(l, 0, row_y);

        /* Offset chip */
        char chip[12];
        if (items[i].offset == 0)      std::snprintf(chip, sizeof(chip), "วันนี้");
        else if (items[i].offset == 1) std::snprintf(chip, sizeof(chip), "พรุ่งนี้");
        else                            std::snprintf(chip, sizeof(chip), "+%d", items[i].offset);

        lv_obj_t *r = lv_label_create(card);
        lv_label_set_text(r, chip);
        lv_obj_set_style_text_color(r, C_WARNING, LV_PART_MAIN);
        lv_obj_set_style_text_font(r, &thai_sarabun_stacked_24, LV_PART_MAIN);
        lv_obj_align(r, LV_ALIGN_TOP_RIGHT, 0, row_y);
    }
}
