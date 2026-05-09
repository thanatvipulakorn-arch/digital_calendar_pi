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
#include "../calendar/thai_calendar.h"
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
/* Phase 2.2.5d — pulled left edge in (theme.cpp anchors at x=24) and
 * stretched bottom down to y=986 (= the right column's bottom) per
 * Lek's red markup. Day cells become roughly 188 x 110 px, fitting the
 * Montserrat 48 day numbers comfortably. */
#define MINI_CARD_W          1368
#define MINI_CARD_H          832

#define MINI_TITLE_H         60
/* MINI_DOW_H bumped to 100 in Phase 2.2.5f. DOW transform_scale is now
 * 2.8x (effective ~67 px) and the labels are pinned to the top of the
 * cell per Lek's spec — was previously 80 px / 2.2x / centred. */
#define MINI_DOW_H           100
#define MINI_LEGEND_H        36
#define MINI_GRID_ROWS       6

#define MINI_INNER_PAD       20

/* Grid line styling — Phase 2.2.5h: brighter + more opaque so the lines
 * read as a real table grid against the busy tulip background. The old
 * slate-blue at 50% opa was too close to the card's translucent navy. */
#define MINI_GRID_COLOR      lv_color_hex(0xE0EFFF)
#define MINI_GRID_OPA        ((lv_opa_t)200)        /* ~78% */
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

/* ──────────── Helpers: explicit grid lines ────────────
 * Phase 2.2.5i — per-cell right+bottom borders looked patchy (the
 * border_side cast + LVGL's content-area math left visible gaps where
 * the line should have continued across DOW↔day and column boundaries).
 * Replacing with first-class horizontal/vertical line objects guarantees
 * pixel-perfect continuous lines. Each "line" is a 1-px lv_obj with
 * bg_color set — no border, no padding, no scrollbar. */
static lv_obj_t *add_vline(lv_obj_t *parent, int x, int y, int h)
{
    lv_obj_t *line = lv_obj_create(parent);
    lv_obj_remove_style_all(line);
    lv_obj_set_pos(line, x, y);
    lv_obj_set_size(line, 1, h);
    lv_obj_set_style_bg_color(line, MINI_GRID_COLOR, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(line, MINI_GRID_OPA, LV_PART_MAIN);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_CLICKABLE);
    return line;
}

static lv_obj_t *add_hline(lv_obj_t *parent, int x, int y, int w)
{
    lv_obj_t *line = lv_obj_create(parent);
    lv_obj_remove_style_all(line);
    lv_obj_set_pos(line, x, y);
    lv_obj_set_size(line, w, 1);
    lv_obj_set_style_bg_color(line, MINI_GRID_COLOR, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(line, MINI_GRID_OPA, LV_PART_MAIN);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_CLICKABLE);
    return line;
}

/* ──────────── Build a single day cell ──────────── */
/* Phase 2.2.5j marker rules per Lek:
 *   วันพระ (regular Buddhist holy day, not also a national holiday)
 *     → number text is yellow + a yellow circle dot below the number
 *   National / major-Buddhist holiday (Songkran, Visakha Puja, etc.)
 *     → cell gets an amber background tint + the Thai holiday name
 *       printed below the number
 *   Today still wins the cell visually (cyan box + navy text).
 */
static void build_day_cell(lv_obj_t *parent, int day, int row, int col,
                           bool is_today, bool is_wanphra, bool is_holiday,
                           const char *holiday_name_th)
{
    int x = MINI_INNER_PAD + col * MINI_CELL_W;
    int y = MINI_INNER_PAD + MINI_GRID_Y + row * MINI_CELL_H;

    /* Transparent positioning container */
    lv_obj_t *cell = lv_obj_create(parent);
    lv_obj_remove_style_all(cell);
    lv_obj_set_size(cell, MINI_CELL_W, MINI_CELL_H);
    lv_obj_set_pos(cell, x, y);
    lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(cell, LV_OBJ_FLAG_CLICKABLE);

    /* Holiday tint (z=1, behind everything else). Skipped on today —
     * the cyan box would cover it anyway. */
    if (is_holiday && !is_today) {
        lv_obj_t *tint = lv_obj_create(cell);
        lv_obj_remove_style_all(tint);
        lv_obj_set_size(tint, MINI_CELL_W - 4, MINI_CELL_H - 4);
        lv_obj_center(tint);
        lv_obj_set_style_bg_color(tint, lv_color_hex(0xBA68C8), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(tint, 70, LV_PART_MAIN);
        lv_obj_set_style_radius(tint, 8, LV_PART_MAIN);
        lv_obj_clear_flag(tint, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(tint, LV_OBJ_FLAG_CLICKABLE);
    }

    /* Today highlight (z=2) */
    if (is_today) {
        lv_obj_t *highlight = lv_obj_create(cell);
        lv_obj_remove_style_all(highlight);
        lv_obj_set_size(highlight, MINI_CELL_W - 16, MINI_CELL_H - 16);
        lv_obj_center(highlight);
        lv_obj_set_style_bg_color(highlight, C_ACCENT, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(highlight, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_radius(highlight, 16, LV_PART_MAIN);
        lv_obj_clear_flag(highlight, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(highlight, LV_OBJ_FLAG_CLICKABLE);
    }

    /* Day number label — Montserrat 48, color by priority */
    lv_obj_t *lbl = lv_label_create(cell);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", day);
    lv_label_set_text(lbl, buf);

    lv_color_t color;
    if (is_today)                       color = C_BG_PRIMARY;                /* navy on cyan */
    else if (is_wanphra && !is_holiday) color = lv_color_hex(0xFFEB3B);      /* bright yellow */
    else if (is_holiday)                color = lv_color_hex(0xE1BEE7);      /* light lilac on purple tint — readable contrast */
    else if (col == 0 || col == 6)      color = lv_color_hex(0xFF7A8C);      /* weekend pink */
    else                                color = C_TEXT_PRIMARY;              /* weekday white */
    lv_obj_set_style_text_color(lbl, color, LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_48, LV_PART_MAIN);
    /* Shift number up if there's a marker (circle or holiday name) below */
    if ((is_wanphra || is_holiday) && !is_today) {
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, -10);
    } else {
        lv_obj_center(lbl);
    }

    /* Wanphra circle (yellow dot below the number) — Phase 2.2.5q: now
     * shown on EVERY วันพระ day, including today and major Buddhist
     * national holidays. On a today-cell the circle overlaps the cyan
     * highlight box at the bottom — yellow on cyan is high contrast and
     * communicates "today is also วันพระ" visibly. When the day is a
     * national holiday the circle sits above the Thai holiday label;
     * otherwise it's centred at the bottom of the cell. */
    if (is_wanphra) {
        int circle_y_off = is_holiday ? -22 : -8;   /* above label vs centred bottom */
        lv_obj_t *circle = lv_obj_create(cell);
        lv_obj_remove_style_all(circle);
        lv_obj_set_size(circle, 12, 12);
        lv_obj_set_style_bg_color(circle, lv_color_hex(0xFFEB3B), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(circle, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_radius(circle, 6, LV_PART_MAIN);
        lv_obj_clear_flag(circle, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(circle, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_align(circle, LV_ALIGN_BOTTOM_MID, 0, circle_y_off);
    }

    /* Holiday name label below the number — Thai text scaled small.
     * thai_sarabun_stacked_24 at 0.55x → effective ~13 px, fits the
     * ~26 px space below a 48 px digit centred at y_offset -10. */
    if (is_holiday && holiday_name_th) {
        lv_obj_t *hlbl = lv_label_create(cell);
        lv_label_set_text(hlbl, holiday_name_th);
        lv_obj_set_style_text_color(hlbl, color, LV_PART_MAIN);
        lv_obj_set_style_text_font(hlbl, &thai_sarabun_stacked_24, LV_PART_MAIN);
        lv_obj_set_style_transform_scale_x(hlbl, 140, LV_PART_MAIN);  /* 0.55x */
        lv_obj_set_style_transform_scale_y(hlbl, 140, LV_PART_MAIN);
        lv_obj_set_width(hlbl, MINI_CELL_W);
        lv_obj_set_style_text_align(hlbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_align(hlbl, LV_ALIGN_BOTTOM_MID, 0, -2);
    }
}

/* Returns the Thai holiday name (lookup'd from English) if (year, month,
 * day) is a Thai national holiday, otherwise NULL. Caller's pointer is
 * valid for the program lifetime (THAI_HOLIDAY_MAP is static const). */
static const char *day_holiday_name_th(int year_ce, int month, int day)
{
    const char *name_en = thai_calendar_fixed_holiday_en(month, day);
    if (!name_en) {
        thai_lunar_t lu = thai_calendar_from_date(year_ce, month, day);
        if (lu.valid) name_en = thai_calendar_holiday_en(&lu);
    }
    if (!name_en) return NULL;
    const char *th = thai_holiday_lookup(name_en);
    return th ? th : name_en;   /* fall back to English if no Thai mapping */
}

/* Returns true if (year, month, day) is a regular Buddhist holy day
 * (วันพระ): waxing 8/15, waning 8, waning 14/15. */
static bool day_is_wanphra(int year_ce, int month, int day)
{
    thai_lunar_t lu = thai_calendar_from_date(year_ce, month, day);
    return lu.valid && thai_calendar_is_buddhist_day(&lu);
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
    lv_obj_set_style_bg_opa(card, 170, LV_PART_MAIN);    /* Phase 2.2.5h: 220 → 170 (~67%) */
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
    lv_obj_set_style_text_font(title, &thai_sarabun_stacked_24, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    /* ── Nav buttons (right side, visual only) ── */
    lv_obj_t *nav_prev = lv_label_create(card);
    lv_label_set_text(nav_prev, "<");
    lv_obj_set_style_text_color(nav_prev, C_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_text_font(nav_prev, &lv_font_montserrat_40, LV_PART_MAIN);
    lv_obj_align(nav_prev, LV_ALIGN_TOP_RIGHT, -130, 0);

    lv_obj_t *nav_home = lv_label_create(card);
    lv_label_set_text(nav_home, LV_SYMBOL_HOME);
    lv_obj_set_style_text_color(nav_home, C_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_text_font(nav_home, &lv_font_montserrat_40, LV_PART_MAIN);
    lv_obj_align(nav_home, LV_ALIGN_TOP_RIGHT, -65, 0);

    lv_obj_t *nav_next = lv_label_create(card);
    lv_label_set_text(nav_next, ">");
    lv_obj_set_style_text_color(nav_next, C_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_text_font(nav_next, &lv_font_montserrat_40, LV_PART_MAIN);
    lv_obj_align(nav_next, LV_ALIGN_TOP_RIGHT, 0, 0);

    /* ── DOW headers (Thai), wrapped in cell containers for grid border ── */
    /* "อา จ อ พ พฤ ศ ส" — short Thai weekday names.
     *
     * Phase 2.2.5f: per-day Thai-tradition colours. อาทิตย์/เสาร์ keep
     * the pink/red they had before (Lek's explicit ask: "ส ใช้แดงเหมือน
     * เดิม"); weekdays get their traditional almsgiving colour so each
     * column is visually distinct. */
    static const uint32_t DOW_COLORS[7] = {
        0xFF7A8C,  /* 0 อา (Sunday)    — red (kept) */
        0xFFD54F,  /* 1 จ  (Monday)    — yellow */
        0xEC407A,  /* 2 อ  (Tuesday)   — pink */
        0x66BB6A,  /* 3 พ  (Wednesday) — green */
        0xFB8C00,  /* 4 พฤ (Thursday)  — orange */
        0x4FC3F7,  /* 5 ศ  (Friday)    — sky blue */
        0xFF7A8C,  /* 6 ส  (Saturday)  — red (per Lek, not the traditional purple) */
    };

    for (int i = 0; i < 7; i++) {
        int x = MINI_INNER_PAD + i * MINI_CELL_W;
        int y = MINI_INNER_PAD + MINI_DOW_Y;

        /* Transparent DOW positioning container (grid drawn separately) */
        lv_obj_t *dow_cell = lv_obj_create(card);
        lv_obj_remove_style_all(dow_cell);
        lv_obj_set_size(dow_cell, MINI_CELL_W, MINI_DOW_H);
        lv_obj_set_pos(dow_cell, x, y);
        lv_obj_clear_flag(dow_cell, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(dow_cell, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *dow = lv_label_create(dow_cell);
        lv_label_set_text(dow, THAI_WEEKDAY_SHORT[i]);
        lv_obj_set_style_text_color(dow, lv_color_hex(DOW_COLORS[i]), LV_PART_MAIN);
        lv_obj_set_style_text_font(dow, &thai_sarabun_stacked_24, LV_PART_MAIN);
        /* Phase 2.2.5n — Lek's insight: day NUMBERS centre perfectly
         * with lv_obj_center because they have no transform_scale; the
         * Thai DOW characters were drifting because LVGL's default
         * transform pivot is the widget's top-left, so scale-up
         * expanded the visual down-and-right. Pinning the pivot to the
         * bbox centre (50%/50%) makes the scaled visual stay centred
         * on the bbox centre, so lv_obj_center then works the same as
         * for the (un-scaled) day numbers. No more empirical y-offset. */
        lv_obj_set_style_transform_scale_x(dow, 717, LV_PART_MAIN);  /* 2.8x */
        lv_obj_set_style_transform_scale_y(dow, 717, LV_PART_MAIN);
        lv_obj_set_style_transform_pivot_x(dow, lv_pct(50), LV_PART_MAIN);
        lv_obj_set_style_transform_pivot_y(dow, lv_pct(50), LV_PART_MAIN);
        lv_obj_center(dow);
    }

    /* ── Day cells ── */
    int row = 0;
    int col = td.dow_first;
    for (int day = 1; day <= td.days_in_month; day++) {
        bool is_today    = (day == td.today);
        bool is_wanphra  = day_is_wanphra(td.year_ce, td.month, day);
        const char *hol  = day_holiday_name_th(td.year_ce, td.month, day);
        bool is_holiday  = (hol != NULL);
        build_day_cell(card, day, row, col, is_today, is_wanphra, is_holiday, hol);

        col++;
        if (col >= 7) {
            col = 0;
            row++;
        }
    }

    /* ── Grid lines (Phase 2.2.5j) ──
     * Phase 2.2.5j adds the missing left + right edges and the top edge
     * (DOW row top), so the grid is now a closed table on all four sides. */
    const int grid_top    = MINI_INNER_PAD + MINI_DOW_Y;            /* top of DOW row */
    const int grid_bot    = MINI_INNER_PAD + MINI_GRID_Y +
                            MINI_GRID_ROWS * MINI_CELL_H;           /* bottom of last day row */
    const int grid_left   = MINI_INNER_PAD;
    const int grid_right  = MINI_INNER_PAD + 7 * MINI_CELL_W;
    const int grid_height = grid_bot - grid_top;
    const int grid_width  = grid_right - grid_left;

    /* 8 vertical lines: left edge (c=0), 6 inner, right edge (c=7) */
    for (int c = 0; c <= 7; c++) {
        add_vline(card, MINI_INNER_PAD + c * MINI_CELL_W, grid_top, grid_height);
    }
    /* 8 horizontal lines: top edge of DOW + DOW divider + 5 day row
     * dividers + bottom edge */
    add_hline(card, grid_left, grid_top, grid_width);          /* top edge */
    for (int r = 0; r <= MINI_GRID_ROWS; r++) {
        int y = MINI_INNER_PAD + MINI_GRID_Y + r * MINI_CELL_H;
        add_hline(card, grid_left, y, grid_width);
    }

    /* Phase 2.2.5p — production cleanup: dropped the "[*] today  [.] event
     * [.] holiday" legend. Day-cell colour + circle + label coding is
     * now self-explanatory; the legend was dead inventory. */
}
