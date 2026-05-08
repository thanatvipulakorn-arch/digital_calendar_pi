/* ================================================================
 * thai_calendar.cpp
 *
 * Thai Lunar Calendar — Implementation (Phase 2.2.3, ported from ESP32)
 *
 * Algorithm:
 *   1. Find year_ce of input date
 *   2. Lookup lunar_year_t entry (must be in table)
 *   3. Compute days since m5_day1
 *   4. Walk through months (29/30 alternation + leap month)
 *   5. Output waxing/waning + day + month
 *
 * Table: 7 years (2025-2031). Pre-m5 dates of 2025 use 2024 fallback
 * (returns valid=false with warning; most user queries are after m5).
 * Coverage extends to ~m5_day1_2032 (March 2032) before next refill needed.
 *
 * Pi port (vs ESP32 original): no source-level changes — pure math.
 * ================================================================ */

#include "thai_calendar.h"
#include <string.h>
#include <stdio.h>

/* ---- Lunar year data ---- */

typedef struct {
    int16_t year_ce;            /* CE year when m5_day1 occurs */
    int32_t m5_day1_epoch;      /* Unix epoch of "ขึ้น 1 เดือน 5" (sunrise ICT) */
    bool    leap_month;         /* true = อธิกมาส (month 8 repeated) */
    bool    leap_day;           /* true = อธิกวาร (month 7 = 30 days) */
} lunar_year_t;

/* Zodiac follows a fixed 12-year cycle starting from year of the Rat.
 * 2020 = ZODIAC_CHUT (Rat, idx 0), so (year_ce - 2020) % 12 maps year → idx.
 * The +12000 keeps the result non-negative for any plausible year_ce. */
static inline uint8_t zodiac_from_year(int16_t year_ce)
{
    return (uint8_t)((year_ce - 2020 + 12000) % 12);
}

/* Epoch calculation (verified via Python datetime, all at sunrise 06:00 ICT):
 *   2025-03-29 = 1743202800   2026-03-19 = 1773874800   2027-04-07 = 1807052400
 *   2028-03-26 = 1837638000   2029-03-15 = 1868223600   2030-04-03 = 1901401200
 *   2031-03-23 = 1931986800
 *
 * Source for 2027-2031: myhora.com per-year ปฏิทินจันทรคติไทย pages
 * (thai-2570 .. thai-2574). Type and m5_day1 cross-checked: all year-to-year
 * deltas equal 354 (regular) or 384 (leap_month), confirming consistency.
 */

static const lunar_year_t LUNAR_TABLE[] = {
    /* year, m5_day1_epoch,  leap_month, leap_day */
    { 2025, 1743202800, false, false },  /* ปีมะเส็ง, ปกติ */
    { 2026, 1773874800, true,  false },  /* ปีมะเมีย, อธิกมาส */
    { 2027, 1807052400, false, false },  /* ปีมะแม,  ปกติ */
    { 2028, 1837638000, false, false },  /* ปีวอก,   ปกติ */
    { 2029, 1868223600, true,  false },  /* ปีระกา,  อธิกมาส */
    { 2030, 1901401200, false, false },  /* ปีจอ,    ปกติ */
    { 2031, 1931986800, true,  false },  /* ปีกุน,   อธิกมาส */
};

static const int LUNAR_TABLE_SIZE = sizeof(LUNAR_TABLE) / sizeof(LUNAR_TABLE[0]);

/* ---- Zodiac name tables ---- */

static const char *ZODIAC_EN[12] = {
    "Rat", "Ox", "Tiger", "Rabbit", "Dragon", "Snake",
    "Horse", "Goat", "Monkey", "Rooster", "Dog", "Pig"
};

/* NOTE: Thai strings require UTF-8 font support in LVGL.
 * Will be used later in Phase A (Thai Font).
 * For now these are reference strings. */
static const char *ZODIAC_TH[12] = {
    "ชวด", "ฉลู", "ขาล", "เถาะ", "มะโรง", "มะเส็ง",
    "มะเมีย", "มะแม", "วอก", "ระกา", "จอ", "กุน"
};

/* ---- Helpers ---- */

/**
 * Number of days in lunar month (zero-indexed):
 *   month_slot: 0=m5, 1=m6, 2=m7, 3=m8, 4=m8_leap (if any), 5=m9, ...
 * For leap-month years: slot 4 = "เดือน 8 หลัง" (30 days)
 * For leap-day years: slot 2 (m7) = 30 days instead of 29
 */
static int month_days_in_slot(int slot, bool leap_month, bool leap_day)
{
    /* Build list of {thai_month, days} dynamically:
     * Start m5 (odd=29), m6 (even=30), m7 (odd=29 or 30 if leap_day),
     * m8 (even=30), [m8_leap (30) if leap_month],
     * m9 (odd=29), m10 (even=30), m11 (odd=29), m12 (even=30),
     * m1 (odd=29), m2 (even=30), m3 (odd=29), m4 (even=30)
     */
    static const int base_seq[12] = {
        /* m5, m6, m7, m8, m9, m10, m11, m12, m1, m2, m3, m4 */
        29,  30, 29, 30, 29, 30,  29,  30,  29, 30, 29, 30
    };

    if (leap_month) {
        /* slots 0..3 same, slot 4 = m8_leap (30), slots 5..12 shift by 1 */
        if (slot <= 3) {
            int d = base_seq[slot];
            if (slot == 2 && leap_day) d = 30;  /* m7 expansion */
            return d;
        }
        if (slot == 4) return 30;   /* leap month 8 (even-like) */
        if (slot >= 5 && slot <= 12) {
            int d = base_seq[slot - 1];
            return d;
        }
        return 0;
    } else {
        if (slot >= 0 && slot <= 11) {
            int d = base_seq[slot];
            if (slot == 2 && leap_day) d = 30;
            return d;
        }
        return 0;
    }
}

/**
 * Translate slot index -> thai month number + leap flag
 */
static void slot_to_thai_month(int slot, bool leap_month,
                               uint8_t *out_month, bool *out_is_leap)
{
    /* Normal year slot->month: 0=5, 1=6, 2=7, 3=8, 4=9, 5=10, 6=11, 7=12,
     *                          8=1, 9=2, 10=3, 11=4
     * Leap year slot->month:   0=5, 1=6, 2=7, 3=8, 4=8 (leap), 5=9,
     *                          6=10, 7=11, 8=12, 9=1, 10=2, 11=3, 12=4
     */
    *out_is_leap = false;
    if (leap_month) {
        if (slot <= 3) *out_month = 5 + slot;  /* 5,6,7,8 */
        else if (slot == 4) { *out_month = 8; *out_is_leap = true; }
        else if (slot <= 7) *out_month = slot + 4;  /* 5->9,6->10,7->11 wait */
        /* slot 5->9, 6->10, 7->11, 8->12, 9->1, 10->2, 11->3, 12->4 */
        else if (slot >= 5 && slot <= 8) *out_month = slot + 4;   /* 9,10,11,12 */
        else if (slot >= 9 && slot <= 12) *out_month = slot - 8;  /* 1,2,3,4 */
    } else {
        /* slot 0->5, 1->6, 2->7, 3->8, 4->9, 5->10, 6->11, 7->12,
         *      8->1, 9->2, 10->3, 11->4 */
        if (slot <= 7) *out_month = 5 + slot;
        else *out_month = slot - 7;
    }
}

/* ---- Main algorithm ---- */

thai_lunar_t thai_calendar_get_lunar(time_t epoch)
{
    thai_lunar_t result = {0};
    result.valid = false;

    if (epoch <= 0) return result;

    /* Find the year entry where m5_day1 <= epoch */
    const lunar_year_t *ent = NULL;
    for (int i = LUNAR_TABLE_SIZE - 1; i >= 0; i--) {
        if (LUNAR_TABLE[i].m5_day1_epoch <= epoch) {
            ent = &LUNAR_TABLE[i];
            break;
        }
    }

    if (!ent) {
        /* Before earliest entry — can't resolve */
        return result;
    }

    /* Check if next entry is valid — we need epoch < next.m5_day1 */
    int idx = ent - LUNAR_TABLE;
    if (idx + 1 < LUNAR_TABLE_SIZE) {
        if (epoch >= LUNAR_TABLE[idx + 1].m5_day1_epoch) {
            ent = &LUNAR_TABLE[idx + 1];
        }
    }

    /* Compute days since m5_day1 (integer days, using ICT midday as rounding) */
    time_t delta_sec = epoch - ent->m5_day1_epoch;
    int days_since = (int)(delta_sec / 86400);

    if (days_since < 0) {
        /* Shouldn't happen since we found the entry */
        return result;
    }

    /* Walk through months */
    int slot = 0;
    int max_slots = ent->leap_month ? 13 : 12;
    int day_in_month = days_since;  /* 0-indexed */

    while (slot < max_slots) {
        int mlen = month_days_in_slot(slot, ent->leap_month, ent->leap_day);
        if (day_in_month < mlen) break;
        day_in_month -= mlen;
        slot++;
    }

    if (slot >= max_slots) {
        /* Past year end — data should be in next year's table */
        return result;
    }

    /* day_in_month is now 0..(mlen-1)
     * day 0..14 = waxing 1..15
     * day 15..(mlen-1) = waning 1..(mlen-15)
     */
    uint8_t month;
    bool is_leap;
    slot_to_thai_month(slot, ent->leap_month, &month, &is_leap);

    result.month = month;
    result.leap_month = is_leap;
    result.year_is_leap = ent->leap_month;  /* year-level flag */
    result.year_ce = ent->year_ce;
    result.zodiac_idx = zodiac_from_year(ent->year_ce);

    if (day_in_month < 15) {
        result.waxing = true;
        result.day = day_in_month + 1;   /* 1..15 */
    } else {
        result.waxing = false;
        result.day = day_in_month - 14;  /* 1..14 or 1..15 */
    }

    result.valid = true;
    return result;
}

thai_lunar_t thai_calendar_from_date(int year_ce, int month, int day)
{
    struct tm tm_in = {0};
    tm_in.tm_year = year_ce - 1900;
    tm_in.tm_mon = month - 1;
    tm_in.tm_mday = day;
    tm_in.tm_hour = 12;  /* midday — avoids DST/rounding issues */
    tm_in.tm_min = 0;
    tm_in.tm_sec = 0;

    time_t t = mktime(&tm_in);
    return thai_calendar_get_lunar(t);
}

/* ---- Name lookup ---- */

const char* thai_calendar_zodiac_en(uint8_t zodiac_idx)
{
    if (zodiac_idx >= 12) return "?";
    return ZODIAC_EN[zodiac_idx];
}

const char* thai_calendar_zodiac_th(uint8_t zodiac_idx)
{
    if (zodiac_idx >= 12) return "?";
    return ZODIAC_TH[zodiac_idx];
}

/* ---- Holy day logic ---- */

bool thai_calendar_is_buddhist_day(const thai_lunar_t *lunar)
{
    if (!lunar || !lunar->valid) return false;

    /* Waxing 8, Waxing 15, Waning 8 always */
    if (lunar->waxing && (lunar->day == 8 || lunar->day == 15)) return true;
    if (!lunar->waxing && lunar->day == 8) return true;

    /* Waning end-of-month:
     *   Odd months (1,3,5,7,9,11) have waning 14 as holy day
     *   Even months (2,4,6,8,10,12) have waning 15 as holy day
     */
    if (!lunar->waxing) {
        bool odd = (lunar->month % 2) == 1;
        if (odd && lunar->day == 14) return true;
        if (!odd && lunar->day == 15) return true;
    }

    return false;
}

const char* thai_calendar_holiday_en(const thai_lunar_t *lunar)
{
    if (!lunar || !lunar->valid) return NULL;

    /* Major Buddhist holidays.
     *
     * Rule: In a ปีอธิกมาส (leap-month year), major holidays shift to
     * the month AFTER their normal month number:
     *   Magha Puja:   normal month 3 → leap year month 4
     *   Visakha Puja: normal month 6 → leap year month 7
     *   Asalha Puja:  normal month 8 → leap year "เดือน 8 หลัง" (the leap m8)
     *   Attami Puja:  normal month 6 → leap year month 7
     *   Ok Phansa:    always month 11
     *   Khao Phansa:  normal แรม 1 month 8 → leap year แรม 1 "เดือน 8 หลัง"
     *
     * So compare against "effective" month = month in leap-year convention.
     */

    /* Waxing 15 holidays (full moon) */
    if (lunar->waxing && lunar->day == 15) {
        if (lunar->year_is_leap) {
            /* Leap year adjustments */
            if (lunar->month == 4) return "Magha Puja";
            if (lunar->month == 7) return "Visakha Puja";
            if (lunar->month == 8 && lunar->leap_month) return "Asalha Puja";
            if (lunar->month == 11) return "Ok Phansa (End of Buddhist Lent)";
        } else {
            /* Normal year */
            if (lunar->month == 3) return "Magha Puja";
            if (lunar->month == 6) return "Visakha Puja";
            if (lunar->month == 8) return "Asalha Puja";
            if (lunar->month == 11) return "Ok Phansa (End of Buddhist Lent)";
        }
    }

    /* Waning 1 = Khao Phansa (Buddhist Lent begins) */
    if (!lunar->waxing && lunar->day == 1) {
        if (lunar->year_is_leap) {
            if (lunar->month == 8 && lunar->leap_month) return "Khao Phansa (Buddhist Lent begins)";
        } else {
            if (lunar->month == 8) return "Khao Phansa (Buddhist Lent begins)";
        }
    }

    /* Waning 8 = Attami Puja (8 days after Visakha) */
    if (!lunar->waxing && lunar->day == 8) {
        if (lunar->year_is_leap) {
            if (lunar->month == 7) return "Attami Puja";
        } else {
            if (lunar->month == 6) return "Attami Puja";
        }
    }

    return NULL;
}

/* ---- Thai Gregorian-fixed holidays ---- */

typedef struct {
    uint8_t month;
    uint8_t day;
    const char *name_en;
} fixed_holiday_t;

static const fixed_holiday_t THAI_FIXED_HOLIDAYS[] = {
    /* January */
    { 1,  1,  "New Year" },
    { 1,  2,  "New Year Holiday" },           /* ครม. special */
    /* April */
    { 4,  6,  "Chakri Day" },
    { 4, 13,  "Songkran" },                   /* Thai New Year day 1 */
    { 4, 14,  "Songkran" },                   /* Family Day */
    { 4, 15,  "Songkran" },                   /* Thai New Year day 3 */
    /* May */
    { 5,  1,  "Labour Day" },
    { 5,  4,  "Coronation Day" },
    { 5, 13,  "Royal Plowing" },
    /* June */
    { 6,  3,  "Queen Suthida Birthday" },
    /* July */
    { 7, 28,  "King Rama X Birthday" },
    /* August */
    { 8, 12,  "Queen Mother Birthday" },      /* Mother's Day */
    /* October */
    { 10, 13, "King Rama IX Memorial" },
    { 10, 23, "Chulalongkorn Day" },
    /* December */
    { 12,  5, "Father's Day" },               /* King Rama IX Birthday */
    { 12, 10, "Constitution Day" },
    { 12, 31, "New Year's Eve" },
};

static const int THAI_FIXED_HOLIDAYS_COUNT =
    sizeof(THAI_FIXED_HOLIDAYS) / sizeof(THAI_FIXED_HOLIDAYS[0]);

const char* thai_calendar_fixed_holiday_en(int month, int day)
{
    if (month < 1 || month > 12 || day < 1 || day > 31) return NULL;
    for (int i = 0; i < THAI_FIXED_HOLIDAYS_COUNT; i++) {
        if (THAI_FIXED_HOLIDAYS[i].month == month &&
            THAI_FIXED_HOLIDAYS[i].day == day) {
            return THAI_FIXED_HOLIDAYS[i].name_en;
        }
    }
    return NULL;
}

/* ---- Formatters ---- */

void thai_calendar_format_en(const thai_lunar_t *lunar, char *buf, size_t buflen)
{
    if (!lunar || !lunar->valid) {
        snprintf(buf, buflen, "(lunar unavailable)");
        return;
    }

    const char *phase = lunar->waxing ? "waxing" : "waning";
    const char *leap  = lunar->leap_month ? "-leap" : "";

    snprintf(buf, buflen, "%s %d, month %d%s",
             phase, lunar->day, lunar->month, leap);
}

void thai_calendar_format_summary_en(const thai_lunar_t *lunar, char *buf, size_t buflen)
{
    if (!lunar || !lunar->valid) {
        snprintf(buf, buflen, "lunar: (out of range)");
        return;
    }

    const char *phase = lunar->waxing ? "waxing" : "waning";
    const char *leap  = lunar->leap_month ? "*" : "";  /* * = leap month */
    const char *zod   = thai_calendar_zodiac_en(lunar->zodiac_idx);

    snprintf(buf, buflen, "%s %d  |  month %d%s  |  year of the %s",
             phase, lunar->day, lunar->month, leap, zod);
}

/* Thai version — requires Thai font for display. UTF-8 encoded. */
void thai_calendar_format_summary_th(const thai_lunar_t *lunar, char *buf, size_t buflen)
{
    if (!lunar || !lunar->valid) {
        snprintf(buf, buflen, "ไม่มีข้อมูล");
        return;
    }

    /* Zodiac names in Thai — must match zodiac_idx enum */
    static const char *zodiac_th[] = {
        "ปีชวด",    /* 0  Rat */
        "ปีฉลู",    /* 1  Ox */
        "ปีขาล",    /* 2  Tiger */
        "ปีเถาะ",   /* 3  Rabbit */
        "ปีมะโรง",  /* 4  Dragon */
        "ปีมะเส็ง", /* 5  Snake */
        "ปีมะเมีย", /* 6  Horse */
        "ปีมะแม",   /* 7  Goat */
        "ปีวอก",    /* 8  Monkey */
        "ปีระกา",   /* 9  Rooster */
        "ปีจอ",     /* 10 Dog */
        "ปีกุน"     /* 11 Pig */
    };

    const char *phase = lunar->waxing ? "ขึ้น" : "แรม";
    const char *leap  = lunar->leap_month ? " หลัง" : "";
    const char *zod   = (lunar->zodiac_idx < 12) ? zodiac_th[lunar->zodiac_idx] : "";

    /* Format: "ขึ้น 8 ค่ำ เดือน 6 | ปีมะเมีย"
     * or:    "แรม 14 ค่ำ เดือน 8 หลัง | ปีมะเมีย" (leap month) */
    snprintf(buf, buflen, "%s %d ค่ำ  เดือน %d%s  |  %s",
             phase, lunar->day, lunar->month, leap, zod);
}
