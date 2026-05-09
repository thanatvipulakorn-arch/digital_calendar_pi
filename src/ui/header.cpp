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

/* Phase 2.2.5p — WiFi SSID cache. Refreshed every 30 ticks (~30 sec)
 * via `iwgetid -r` to keep the popen overhead off the per-second clock
 * path. Empty string means no WiFi (e.g., Pi is on Ethernet only). */
static char g_wifi_ssid[64] = "";
static int  g_wifi_poll_cnt = 0;

static void poll_wifi_ssid(void)
{
    /* `iwgetid` (wireless-tools) isn't installed on Pi OS Trixie — it
     * ships NetworkManager instead. nmcli's machine-readable output
     * "active:ssid", filtered with awk for the active=yes row, gives
     * us the connected SSID even if it contains colons or spaces.
     * popen/pclose are POSIX (not std::) — keep them unqualified. */
    FILE *p = popen(
        "nmcli -t -f active,ssid dev wifi 2>/dev/null"
        " | awk -F: '$1==\"yes\"{print $2; exit}'",
        "r");
    if (!p) return;
    char buf[64] = "";
    if (std::fgets(buf, sizeof(buf), p)) {
        char *nl = std::strchr(buf, '\n');
        if (nl) *nl = '\0';
    }
    pclose(p);
    std::strncpy(g_wifi_ssid, buf, sizeof(g_wifi_ssid) - 1);
    g_wifi_ssid[sizeof(g_wifi_ssid) - 1] = '\0';
}

/* ──────────── Build ──────────── */
extern "C" void header_build(lv_obj_t *parent, int x, int y, int w, int h)
{
    lv_obj_t *bar = lv_obj_create(parent);
    g_bar = bar;
    lv_obj_set_size(bar, w, h);
    lv_obj_set_pos(bar, x, y);
    /* Translucent — let the tulip tone through, but readable */
    lv_obj_set_style_bg_color(bar, C_BG_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, 160, LV_PART_MAIN);    /* Phase 2.2.5h: 200 → 160 (~63%) */
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
    lv_obj_set_style_text_font(g_lbl_weekday, &thai_sarabun_stacked_24, LV_PART_MAIN);
    lv_obj_align(g_lbl_weekday, LV_ALIGN_TOP_LEFT, 0, 0);

    g_lbl_date = lv_label_create(bar);
    lv_label_set_text(g_lbl_date, "—");
    lv_obj_set_style_text_color(g_lbl_date, C_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(g_lbl_date, &thai_sarabun_stacked_24, LV_PART_MAIN);
    lv_obj_align(g_lbl_date, LV_ALIGN_TOP_LEFT, 0, 26);

    g_lbl_lunar = lv_label_create(bar);
    lv_label_set_text(g_lbl_lunar, "—");
    lv_obj_set_style_text_color(g_lbl_lunar, C_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_set_style_text_font(g_lbl_lunar, &thai_sarabun_stacked_24, LV_PART_MAIN);
    lv_obj_align(g_lbl_lunar, LV_ALIGN_TOP_LEFT, 0, 54);

    /* ── Centre column: clock + sync status ──
     * Phase 2.2.5c: clock blown up via transform_scale because LVGL's
     * builtin Montserrat tops out at 48 px. Asymmetric scale (wider than
     * tall) per Lek's spec — there's more horizontal room than vertical
     * inside the 130 px header. 256 = 1.0x. */
    g_lbl_clock = lv_label_create(bar);
    lv_label_set_text(g_lbl_clock, "00:00:00");
    lv_obj_set_style_text_color(g_lbl_clock, C_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(g_lbl_clock, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_transform_scale_x(g_lbl_clock, 460, LV_PART_MAIN);  /* 1.8x */
    lv_obj_set_style_transform_scale_y(g_lbl_clock, 384, LV_PART_MAIN);  /* 1.5x */
    /* Phase 2.2.5s — same pivot fix as the DOW labels: default top-left
     * pivot was pushing the scaled clock to the right of the bar centre.
     * 50%/50% keeps the scaled visual centred on the bbox centre. */
    lv_obj_set_style_transform_pivot_x(g_lbl_clock, lv_pct(50), LV_PART_MAIN);
    lv_obj_set_style_transform_pivot_y(g_lbl_clock, lv_pct(50), LV_PART_MAIN);
    lv_obj_align(g_lbl_clock, LV_ALIGN_TOP_MID, 0, 8);

    g_lbl_sync = lv_label_create(bar);
    /* Phase 2.2.5s — recolor enabled so the WiFi icon can be green when
     * connected / red when offline, independent of the surrounding text
     * colour. Format: "#RRGGBB <icon># <ssid> - <ntp status>". */
    lv_label_set_recolor(g_lbl_sync, true);
    lv_label_set_text(g_lbl_sync, "#F44336 " LV_SYMBOL_WIFI "# (offline)");
    lv_obj_set_style_text_color(g_lbl_sync, C_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(g_lbl_sync, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_align(g_lbl_sync, LV_ALIGN_TOP_MID, 0, 90);

    /* ── Right: วันพระ indicator ── */
    g_lbl_wanphra = lv_label_create(bar);
    lv_label_set_text(g_lbl_wanphra, "");
    lv_obj_set_style_text_color(g_lbl_wanphra, C_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(g_lbl_wanphra, &thai_sarabun_stacked_24, LV_PART_MAIN);
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

    /* WiFi SSID + NTP sync — combined into one label below the clock
     * (Phase 2.2.5p). SSID is re-polled every ~30 sec since `iwgetid`
     * is a popen and shouldn't run every tick; NTP is a cheap fopen
     * and runs every sec. The label is only rewritten when the merged
     * string changes, so LVGL doesn't redraw on every tick. */
    if (g_lbl_sync && sec_changed) {
        static char last[160] = "";
        if (g_wifi_poll_cnt-- <= 0) {
            g_wifi_poll_cnt = 30;
            poll_wifi_ssid();
        }
        FILE *f = std::fopen("/run/systemd/timesync/synchronized", "r");
        bool synced = (f != nullptr);
        if (f) std::fclose(f);

        char cur[200];
        const char *ntp = synced ? "NTP synced" : "NTP pending";
        const bool wifi_up = (g_wifi_ssid[0] != '\0');
        const char *icon_col = wifi_up ? "4CAF50" : "F44336";   /* green / red */
        if (wifi_up) {
            std::snprintf(cur, sizeof(cur),
                "#%s " LV_SYMBOL_WIFI "# %s  -  %s",
                icon_col, g_wifi_ssid, ntp);
        } else {
            std::snprintf(cur, sizeof(cur),
                "#%s " LV_SYMBOL_WIFI "# (offline)  -  %s",
                icon_col, ntp);
        }
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
