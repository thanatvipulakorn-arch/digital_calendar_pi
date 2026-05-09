/* ================================================================
 * mini_calendar.cpp - Mini Calendar widget implementation
 *
 * Phase 2.2.1: Hardcoded "พฤษภาคม 2569" (May 2026)
 * Phase 2.2.4: Reads system time — month, year (BE), today, DOW, days
 *              are now derived from time(NULL) + localtime_r().
 *
 * Layout (within card):
 *   ┌──────────────────────────────────────────────┐
 *   │  <month>  <BE-year>     <  🏠  >             │  Title row (40px)
 *   ├──────────────────────────────────────────────┤
 *   │  อา    จ    อ    พ    พฤ    ศ    ส           │  DOW row (36px)
 *   ├──────────────────────────────────────────────┤
 *   │  ...   <today highlighted in cyan>   ...     │  Grid (6 rows)
 *   └──────────────────────────────────────────────┘
 *
 * Cell size: ~85x60 px (calendar = ~700x500)
 * ================================================================ */

#include "mini_calendar.h"
#include "theme.h"
#include "../assets/thai_fonts.h"
#include "../assets/thai_strings.h"
#include <cstdio>
#include <cstdbool>
#include <ctime>

/* ──────────── Current-date snapshot (Phase 2.2.4) ──────────── */
typedef struct {
    int year_ce;       /* e.g. 2026 */
    int year_be;       /* year_ce + 543, e.g. 2569 */
    int month;         /* 1..12 */
    int today;         /* 1..31 */
    int dow_first;     /* day-of-week of the 1st (0=Sun, 6=Sat) */
    int days_in_month; /* 28..31 */
} cal_today_t;

static bool is_gregorian_leap(int year_ce)
{
    return (year_ce % 4 == 0 && year_ce % 100 != 0) || (year_ce % 400 == 0);
}

static cal_today_t cal_today_snapshot(void)
{
    cal_today_t t;
    time_t now = time(NULL);
    struct tm tm_local;
    localtime_r(&now, &tm_local);

    t.year_ce = tm_local.tm_year + 1900;
    t.year_be = t.year_ce + 543;
    t.month   = tm_local.tm_mon + 1;
    t.today   = tm_local.tm_mday;

    /* DOW of day 1 — build a fresh tm at the 1st of the month at midday
     * (midday avoids any DST/rounding edge), then mktime() normalises
     * tm_wday for us. */
    struct tm tm1 = {};
    tm1.tm_year = tm_local.tm_year;
    tm1.tm_mon  = tm_local.tm_mon;
    tm1.tm_mday = 1;
    tm1.tm_hour = 12;
    tm1.tm_isdst = -1;
    mktime(&tm1);
    t.dow_first = tm1.tm_wday;   /* 0=Sun..6=Sat */

    static const int dim[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    t.days_in_month = dim[t.month - 1];
    if (t.month == 2 && is_gregorian_leap(t.year_ce)) {
        t.days_in_month = 29;
    }
    return t;
}

/* ──────────── Layout tokens ──────────── */
#define MINI_CARD_W          700
#define MINI_CARD_H          500

#define MINI_TITLE_H         50
#define MINI_DOW_H           42
#define MINI_LEGEND_H        30
#define MINI_GRID_ROWS       6

#define MINI_INNER_PAD       16
#define MINI_INNER_W         (MINI_CARD_W - 2 * MINI_INNER_PAD)
#define MINI_CELL_W          (MINI_INNER_W / 7)
#define MINI_GRID_AVAIL      (MINI_CARD_H - 2 * MINI_INNER_PAD - MINI_TITLE_H - MINI_DOW_H - MINI_LEGEND_H)
#define MINI_CELL_H          (MINI_GRID_AVAIL / MINI_GRID_ROWS)

#define MINI_DOW_Y           (MINI_TITLE_H)
#define MINI_GRID_Y          (MINI_DOW_Y + MINI_DOW_H)
#define MINI_LEGEND_Y        (MINI_CARD_H - 2 * MINI_INNER_PAD - MINI_LEGEND_H)

/* ──────────── Title ──────────── */
/* Phase 2.2.2: ใช้ Thai จาก thai_strings.h */
/* "พฤษภาคม 2569" — May 2026 = พ.ศ. 2569 */

/* ──────────── Build a single day cell ──────────── */
static void build_day_cell(lv_obj_t *parent, int day, int row, int col, bool is_today)
{
    int x = MINI_INNER_PAD + col * MINI_CELL_W;
    int y = MINI_INNER_PAD + MINI_GRID_Y + row * MINI_CELL_H;

    /* Today: filled cyan rounded box */
    if (is_today) {
        lv_obj_t *highlight = lv_obj_create(parent);
        lv_obj_set_size(highlight, MINI_CELL_W - 8, MINI_CELL_H - 8);
        lv_obj_set_pos(highlight, x + 4, y + 4);
        lv_obj_set_style_bg_color(highlight, C_ACCENT, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(highlight, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(highlight, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(highlight, 12, LV_PART_MAIN);
        lv_obj_clear_flag(highlight, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(highlight, LV_OBJ_FLAG_CLICKABLE);
    }

    /* Day number label */
    lv_obj_t *lbl = lv_label_create(parent);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", day);
    lv_label_set_text(lbl, buf);

    /* Color: today = navy on cyan, weekend = pink, weekday = white */
    lv_color_t color;
    if (is_today) {
        color = C_BG_PRIMARY;  /* dark navy on cyan = high contrast */
    } else if (col == 0 || col == 6) {
        /* Weekend: salmon pink */
        color = lv_color_hex(0xFF7A8C);
    } else {
        color = C_TEXT_PRIMARY;
    }
    lv_obj_set_style_text_color(lbl, color, LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, LV_PART_MAIN);

    /* Center the number in the cell */
    lv_obj_set_size(lbl, MINI_CELL_W, MINI_CELL_H);
    lv_obj_set_pos(lbl, x, y);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_pad_top(lbl, MINI_CELL_H / 2 - 14, LV_PART_MAIN);
}

/* ──────────── Public API ──────────── */
extern "C" void mini_calendar_build(lv_obj_t *parent, int x, int y)
{
    /* Read system time once at build (Phase 2.2.4).
     * Phase 2.2.4-MIDNIGHT will rebuild via lv_timer when the day rolls. */
    const cal_today_t td = cal_today_snapshot();

    /* ── Card ──
     * Phase 2.2.5: card is now translucent so the tulip background shows
     * through. 220/255 ≈ 86% opacity — readable text, hint of pink/green
     * underneath. Tweak between 200..240 if too see-through or too solid. */
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, MINI_CARD_W, MINI_CARD_H);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_style_bg_color(card, C_BG_SECONDARY, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, 220, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, C_BORDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, MINI_INNER_PAD, LV_PART_MAIN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* ── Title ── */
    /* Thai month name + Buddhist year, e.g. "พฤษภาคม 2569" */
    lv_obj_t *title = lv_label_create(card);
    char title_buf[64];
    snprintf(title_buf, sizeof(title_buf), "%s %d",
             THAI_MONTH_FULL[td.month],   /* index 1..12 = Jan..Dec */
             td.year_be);
    lv_label_set_text(title, title_buf);
    lv_obj_set_style_text_color(title, C_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &thai_sarabun_24, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    /* ── Nav buttons (right side, visual only) ── */
    lv_obj_t *nav_prev = lv_label_create(card);
    lv_label_set_text(nav_prev, "<");
    lv_obj_set_style_text_color(nav_prev, C_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_text_font(nav_prev, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_align(nav_prev, LV_ALIGN_TOP_RIGHT, -110, 0);

    lv_obj_t *nav_home = lv_label_create(card);
    lv_label_set_text(nav_home, LV_SYMBOL_HOME);
    lv_obj_set_style_text_color(nav_home, C_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_text_font(nav_home, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_align(nav_home, LV_ALIGN_TOP_RIGHT, -55, 0);

    lv_obj_t *nav_next = lv_label_create(card);
    lv_label_set_text(nav_next, ">");
    lv_obj_set_style_text_color(nav_next, C_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_text_font(nav_next, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_align(nav_next, LV_ALIGN_TOP_RIGHT, 0, 0);

    /* ── DOW headers (Thai) ── */
    /* "อา จ อ พ พฤ ศ ส" — short Thai weekday names */
    for (int i = 0; i < 7; i++) {
        lv_obj_t *dow = lv_label_create(card);
        lv_label_set_text(dow, THAI_WEEKDAY_SHORT[i]);
        /* Sun (0) and Sat (6) = pink, others = muted */
        lv_color_t c = (i == 0 || i == 6) ? lv_color_hex(0xFF7A8C) : C_TEXT_MUTED;
        lv_obj_set_style_text_color(dow, c, LV_PART_MAIN);
        lv_obj_set_style_text_font(dow, &thai_sarabun_24, LV_PART_MAIN);

        /* Center within cell */
        lv_obj_set_size(dow, MINI_CELL_W, MINI_DOW_H);
        lv_obj_set_pos(dow, i * MINI_CELL_W, MINI_DOW_Y);
        lv_obj_set_style_text_align(dow, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    }

    /* ── Day cells ── */
    int row = 0;
    int col = td.dow_first;
    for (int day = 1; day <= td.days_in_month; day++) {
        bool is_today = (day == td.today);
        build_day_cell(card, day, row, col, is_today);

        col++;
        if (col >= 7) {
            col = 0;
            row++;
        }
    }

    /* ── Legend (bottom) ── */
    lv_obj_t *legend = lv_label_create(card);
    lv_label_set_text(legend, "[*] today    [.] event    [.] holiday");
    lv_obj_set_style_text_color(legend, C_TEXT_HINT, LV_PART_MAIN);
    lv_obj_set_style_text_font(legend, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(legend, LV_ALIGN_BOTTOM_LEFT, 0, 0);
}
