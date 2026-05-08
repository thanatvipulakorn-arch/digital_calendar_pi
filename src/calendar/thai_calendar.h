/* ================================================================
 * thai_calendar.h
 *
 * Thai Lunar Calendar (ปฏิทินจันทรคติไทย) — Phase 2.2.3 (ported from ESP32)
 *
 * Data: 7 years (2568-2574 / 2025-2031)
 * Extensible to 20 years in future (just add entries to LUNAR_TABLE)
 *
 * Key concepts:
 *   - Lunar year starts at "ขึ้น 1 ค่ำ เดือน 5" (~late March / early April)
 *   - Zodiac changes at the same day (Brahmin tradition)
 *   - Odd months (1,3,5,7,9,11) = 29 days
 *   - Even months (2,4,6,8,10,12) = 30 days
 *   - Leap month (อธิกมาส): month 8 repeated — "เดือน 8 หลัง" (30 days)
 *   - Leap day (อธิกวาร): month 7 becomes 30 days instead of 29
 *
 * Reference truth (myhora.com verified, all sunrise ICT):
 *   2568 (ปีมะเส็ง, normal):     m5_day1 = Sat 29 Mar 2025 ~06:16
 *   2569 (ปีมะเมีย, leap-month): m5_day1 = Thu 19 Mar 2026 ~06:23
 *   2570 (ปีมะแม,  normal):     m5_day1 = Wed 07 Apr 2027 ~06:00
 *   2571 (ปีวอก,   normal):     m5_day1 = Sun 26 Mar 2028 ~06:18
 *   2572 (ปีระกา,  leap-month): m5_day1 = Thu 15 Mar 2029 ~06:26
 *   2573 (ปีจอ,    normal):     m5_day1 = Wed 03 Apr 2030 ~06:13
 *   2574 (ปีกุน,   leap-month): m5_day1 = Sun 23 Mar 2031 ~06:21
 *
 * Pi port (vs ESP32 original):
 *   - Removed #include <Arduino.h>; using <stdint.h> + <stdbool.h> + <time.h>
 *   - Otherwise identical (pure math, no platform calls)
 * ================================================================ */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Public types ---- */

typedef struct {
    bool    waxing;       /* true = ขึ้น (waxing), false = แรม (waning) */
    uint8_t day;          /* 1..15 waxing, 1..14 or 1..15 waning */
    uint8_t month;        /* 1..12 (or 8 for both normal/leap m8) */
    bool    leap_month;   /* true if THIS is "เดือน 8 หลัง" */
    bool    year_is_leap; /* true if the YEAR is ปีอธิกมาส (has leap month) */
    int16_t year_ce;      /* Lunar year in CE (same as Gregorian year most of the time) */
    uint8_t zodiac_idx;   /* 0..11 (0=ชวด, 6=มะเมีย, etc.) */
    bool    valid;        /* false if out of table range */
} thai_lunar_t;

/* ---- Zodiac constants ---- */

enum {
    ZODIAC_CHUT = 0,    /* ชวด   Rat */
    ZODIAC_CHALU,       /* ฉลู   Ox */
    ZODIAC_KHAL,        /* ขาล   Tiger */
    ZODIAC_THOR,        /* เถาะ  Rabbit */
    ZODIAC_MARONG,      /* มะโรง Dragon */
    ZODIAC_MASENG,      /* มะเส็ง Snake */
    ZODIAC_MAMIA,       /* มะเมีย Horse */
    ZODIAC_MAMAE,       /* มะแม  Goat */
    ZODIAC_WOK,         /* วอก   Monkey */
    ZODIAC_RAKA,        /* ระกา  Rooster */
    ZODIAC_CHOR,        /* จอ    Dog */
    ZODIAC_KUN          /* กุน   Pig */
};

/* ---- Public API ---- */

/**
 * Compute lunar date from Unix epoch (seconds since 1970).
 * Returns thai_lunar_t. Check .valid before using.
 */
thai_lunar_t thai_calendar_get_lunar(time_t epoch);

/**
 * Compute lunar date from broken-down Gregorian date (simpler interface).
 * year_ce = 2025, 2026, etc.
 */
thai_lunar_t thai_calendar_from_date(int year_ce, int month, int day);

/**
 * Get zodiac name.
 * - thai_calendar_zodiac_en(idx) -> "Horse", "Snake", ...
 * - thai_calendar_zodiac_th(idx) -> "มะเมีย", "มะเส็ง", ... (Thai, for Phase A font)
 */
const char* thai_calendar_zodiac_en(uint8_t zodiac_idx);
const char* thai_calendar_zodiac_th(uint8_t zodiac_idx);

/**
 * Check if lunar date is Buddhist holy day (วันพระ):
 *   waxing 8, waxing 15, waning 8, waning 14/15
 */
bool thai_calendar_is_buddhist_day(const thai_lunar_t *lunar);

/**
 * Get name of major Buddhist holiday, or NULL if not a holiday.
 * Examples: "Magha Puja", "Visakha Puja", "Asalha Puja", "Khao Phansa", "Ok Phansa"
 * (Thai names in zodiac_th version)
 */
const char* thai_calendar_holiday_en(const thai_lunar_t *lunar);

/**
 * Get name of Thai Gregorian-fixed holiday for a given date, or NULL.
 * Examples: "New Year", "Chakri Day", "Songkran", "Labour Day",
 *           "Coronation Day", "Father's Day", "Constitution Day"
 * Note: These do not overlap with Buddhist lunar holidays.
 */
const char* thai_calendar_fixed_holiday_en(int month, int day);

/**
 * Format lunar date as English string.
 * Output: "waxing 8, month 6" or "waning 14, month 5"
 * Buffer size: at least 40 bytes.
 */
void thai_calendar_format_en(const thai_lunar_t *lunar, char *buf, size_t buflen);

/**
 * Compact one-line summary: "waxing 8 | month 6 | year of the Horse"
 * Buffer size: at least 64 bytes.
 */
void thai_calendar_format_summary_en(const thai_lunar_t *lunar, char *buf, size_t buflen);

/**
 * Compact Thai summary: "ขึ้น 8 ค่ำ เดือน 6 | ปีมะเมีย"
 * Requires Thai font for display.
 * Buffer: at least 80 bytes (Thai is multi-byte UTF-8).
 */
void thai_calendar_format_summary_th(const thai_lunar_t *lunar, char *buf, size_t buflen);

#ifdef __cplusplus
}
#endif
