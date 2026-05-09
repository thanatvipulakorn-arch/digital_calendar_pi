/* ================================================================
 * header.cpp — Top header bar (Phase 2.2.6)
 *
 * Ported from ESP32 ui_home.cpp build_today_header() + ui_update_home()
 * header section. Logic is the same; only platform calls (Serial.printf
 * → fprintf, time access via system time) differ.
 *
 * Update cadence (driven by header_tick @ 1 Hz):
 *   - clock         every second
 *   - sync status   when string changes
 *   - weekday/date/lunar/wanphra  on midnight rollover only (day_changed)
 * ================================================================ */
#include "header.h"
#include "theme.h"
#include "../assets/thai_fonts.h"
#include "../assets/thai_strings.h"
#include "../calendar/thai_calendar.h"

#include <cstdio>
#include <cstring>
#include <ctime>

/* ──────────── Module-level label handles ──────────── */
static lv_obj_t *g_bar         = nullptr;
static lv_obj_t *g_lbl_weekday = nullptr;
static lv_obj_t *g_lbl_date    = nullptr;
static lv_obj_t *g_lbl_lunar   = nullptr;
static lv_obj_t *g_lbl_clock   = nullptr;
static lv_obj_t *g_lbl_sync    = nullptr;
static lv_obj_t *g_lbl_wanphra = nullptr;
static lv_obj_t *g_dot_wanphra = nullptr;

/* tick state */
static int g_last_sec  = -1;
static int g_last_yday = -1;
static int g_last_wday = -1;

/* ──────────── Build ──────────── */
extern "C" void header_build(lv_obj_t *parent, int x, int y, int w, int h)
{
    lv_obj_t *bar = lv_obj_create(parent);
    g_bar = bar;
    lv_obj_set_size(bar, w, h);
    lv_obj_set_pos(bar, x, y);
    /* Translucent — let the tulip tone through, but readable */
    lv_obj_set_style_bg_color(bar, C_BG_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, 200, LV_PART_MAIN);    /* 78% */
    lv_obj_set_style_border_color(bar, C_BORDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(bar, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(bar, 12, LV_PART_MAIN);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    /* ── Left column: weekday / date / lunar (stacked) ── */
    g_lbl_weekday = lv_label_create(bar);
    lv_label_set_text(g_lbl_weekday, "—");
    lv_obj_set_style_text_color(g_lbl_weekday, C_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_set_style_text_font(g_lbl_weekday, &thai_sarabun_24, LV_PART_MAIN);
    lv_obj_align(g_lbl_weekday, LV_ALIGN_TOP_LEFT, 0, 0);

    g_lbl_date = lv_label_create(bar);
    lv_label_set_text(g_lbl_date, "—");
    lv_obj_set_style_text_color(g_lbl_date, C_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(g_lbl_date, &thai_sarabun_24, LV_PART_MAIN);
    lv_obj_align(g_lbl_date, LV_ALIGN_TOP_LEFT, 0, 26);

    g_lbl_lunar = lv_label_create(bar);
    lv_label_set_text(g_lbl_lunar, "—");
    lv_obj_set_style_text_color(g_lbl_lunar, C_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_set_style_text_font(g_lbl_lunar, &thai_sarabun_24, LV_PART_MAIN);
    lv_obj_align(g_lbl_lunar, LV_ALIGN_TOP_LEFT, 0, 54);

    /* ── Centre column: clock + sync status ── */
    g_lbl_clock = lv_label_create(bar);
    lv_label_set_text(g_lbl_clock, "00:00:00");
    lv_obj_set_style_text_color(g_lbl_clock, C_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(g_lbl_clock, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_align(g_lbl_clock, LV_ALIGN_TOP_MID, 0, 8);

    g_lbl_sync = lv_label_create(bar);
    lv_label_set_text(g_lbl_sync, "");
    lv_obj_set_style_text_color(g_lbl_sync, C_TEXT_HINT, LV_PART_MAIN);
    lv_obj_set_style_text_font(g_lbl_sync, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(g_lbl_sync, LV_ALIGN_TOP_MID, 0, 64);

    /* ── Right: วันพระ indicator ── */
    g_lbl_wanphra = lv_label_create(bar);
    lv_label_set_text(g_lbl_wanphra, "");
    lv_obj_set_style_text_color(g_lbl_wanphra, C_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(g_lbl_wanphra, &thai_sarabun_24, LV_PART_MAIN);
    lv_obj_align(g_lbl_wanphra, LV_ALIGN_TOP_RIGHT, -16, 16);

    g_dot_wanphra = lv_obj_create(bar);
    lv_obj_set_size(g_dot_wanphra, 16, 16);
    lv_obj_set_style_radius(g_dot_wanphra, 8, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_dot_wanphra, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g_dot_wanphra, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(g_dot_wanphra, C_SUCCESS, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_dot_wanphra, 0, LV_PART_MAIN);
    lv_obj_clear_flag(g_dot_wanphra, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(g_dot_wanphra, LV_OBJ_FLAG_CLICKABLE);

    /* Force first refresh */
    g_last_sec  = -1;
    g_last_yday = -1;
    g_last_wday = -1;
    header_tick();
}

/* ──────────── Tick ──────────── */
extern "C" void header_tick(void)
{
    if (!g_bar) return;

    time_t now = time(NULL);
    struct tm tm_local;
    localtime_r(&now, &tm_local);

    bool sec_changed  = (tm_local.tm_sec  != g_last_sec);
    bool day_changed  = (tm_local.tm_yday != g_last_yday);
    bool wday_changed = (tm_local.tm_wday != g_last_wday);
    g_last_sec  = tm_local.tm_sec;
    g_last_yday = tm_local.tm_yday;
    g_last_wday = tm_local.tm_wday;

    /* Clock — every second */
    if (g_lbl_clock && sec_changed) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
                      tm_local.tm_hour, tm_local.tm_min, tm_local.tm_sec);
        lv_label_set_text(g_lbl_clock, buf);
    }

    /* Weekday — midnight only */
    if (g_lbl_weekday && wday_changed) {
        int wd = tm_local.tm_wday;
        if (wd < 0 || wd > 6) wd = 0;
        lv_label_set_text(g_lbl_weekday, THAI_WEEKDAY_FULL[wd]);
    }

    /* BE date — midnight only */
    if (g_lbl_date && day_changed) {
        int y = tm_local.tm_year + 1900 + 543;
        int m = tm_local.tm_mon + 1;
        int d = tm_local.tm_mday;
        const char *mon = (m >= 1 && m <= 12) ? THAI_MONTH_FULL[m] : "";
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%d %s %d", d, mon, y);
        lv_label_set_text(g_lbl_date, buf);
    }

    /* Lunar — midnight only */
    if (g_lbl_lunar && day_changed) {
        thai_lunar_t lunar = thai_calendar_get_lunar(now);
        char buf[96];
        thai_calendar_format_summary_th(&lunar, buf, sizeof(buf));
        lv_label_set_text(g_lbl_lunar, buf);
    }

    /* NTP sync status — Linux: read /run/systemd/timesync/synchronized */
    if (g_lbl_sync && sec_changed) {
        static char last[32] = "";
        FILE *f = std::fopen("/run/systemd/timesync/synchronized", "r");
        const char *cur = f ? "NTP synced" : "NTP pending";
        if (f) std::fclose(f);
        if (std::strcmp(cur, last) != 0) {
            std::strncpy(last, cur, sizeof(last) - 1);
            last[sizeof(last) - 1] = '\0';
            lv_label_set_text(g_lbl_sync, cur);
        }
    }

    /* วันพระ — midnight only. Look-ahead 14 days finds the next holy day. */
    if (g_lbl_wanphra && day_changed) {
        char wbuf[64] = "";
        thai_lunar_t lun_today = thai_calendar_get_lunar(now);
        bool today_is_buddhist =
            lun_today.valid && thai_calendar_is_buddhist_day(&lun_today);

        if (today_is_buddhist) {
            std::snprintf(wbuf, sizeof(wbuf), "%s วันนี้", THAI_WAN_PHRA);
        } else {
            int found = -1;
            for (int n = 1; n <= 14; n++) {
                time_t fut = now + (time_t)n * 86400;
                thai_lunar_t lun = thai_calendar_get_lunar(fut);
                if (lun.valid && thai_calendar_is_buddhist_day(&lun)) {
                    found = n;
                    break;
                }
            }
            if (found == 1) {
                std::snprintf(wbuf, sizeof(wbuf), "%s พรุ่งนี้", THAI_WAN_PHRA);
            } else if (found > 1) {
                std::snprintf(wbuf, sizeof(wbuf), "%s: อีก %d วัน",
                              THAI_WAN_PHRA, found);
            }
        }
        lv_label_set_text(g_lbl_wanphra, wbuf);

        if (g_dot_wanphra) {
            lv_obj_set_style_bg_color(g_dot_wanphra,
                today_is_buddhist ? C_WARNING : C_SUCCESS, LV_PART_MAIN);
            lv_obj_update_layout(g_lbl_wanphra);
            lv_obj_align_to(g_dot_wanphra, g_lbl_wanphra,
                            LV_ALIGN_OUT_LEFT_MID, -6, 0);
        }
    }
}
