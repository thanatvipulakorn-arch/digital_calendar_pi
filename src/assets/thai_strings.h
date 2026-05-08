/* ================================================================
 * thai_strings.h
 *
 * Thai string tables for UI - ported from ESP32 ui_thai_strings.h
 *
 * Differences from ESP32 version:
 *   - Removed #include <Arduino.h>
 *   - Added <string.h> for strncmp/strlen
 *   - Same UTF-8 encoded strings
 *
 * Requires thai_sarabun_24 font for rendering.
 * ================================================================ */
#pragma once

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Weekday names ---- */

/* Short form (2 chars max, for narrow cells) */
/*                                    Sun   Mon   Tue   Wed   Thu   Fri   Sat */
static const char *THAI_WEEKDAY_SHORT[]  = {"อา", "จ",  "อ",  "พ",  "พฤ", "ศ",  "ส"};

/* Full form */
static const char *THAI_WEEKDAY_FULL[] = {
    "วันอาทิตย์",
    "วันจันทร์",
    "วันอังคาร",
    "วันพุธ",
    "วันพฤหัสบดี",
    "วันศุกร์",
    "วันเสาร์"
};

/* ---- Month names ---- */

/* Short form */
static const char *THAI_MONTH_SHORT[] = {
    "",
    "ม.ค.", "ก.พ.", "มี.ค.", "เม.ย.",
    "พ.ค.", "มิ.ย.", "ก.ค.", "ส.ค.",
    "ก.ย.", "ต.ค.", "พ.ย.", "ธ.ค."
};

/* Full form (for Month view title) */
static const char *THAI_MONTH_FULL[] = {
    "",
    "มกราคม",  "กุมภาพันธ์", "มีนาคม",   "เมษายน",
    "พฤษภาคม", "มิถุนายน",  "กรกฎาคม", "สิงหาคม",
    "กันยายน", "ตุลาคม",    "พฤศจิกายน","ธันวาคม"
};

/* ---- Zodiac (Thai) ---- */
static const char *THAI_ZODIAC_FULL[] = {
    "ปีชวด",     /* 0 Rat */
    "ปีฉลู",     /* 1 Ox */
    "ปีขาล",     /* 2 Tiger */
    "ปีเถาะ",    /* 3 Rabbit */
    "ปีมะโรง",   /* 4 Dragon */
    "ปีมะเส็ง",  /* 5 Snake */
    "ปีมะเมีย",  /* 6 Horse */
    "ปีมะแม",    /* 7 Goat */
    "ปีวอก",     /* 8 Monkey */
    "ปีระกา",    /* 9 Rooster */
    "ปีจอ",      /* 10 Dog */
    "ปีกุน"      /* 11 Pig */
};

/* ---- Lunar phase ---- */
static const char *THAI_LUNAR_WAXING = "ขึ้น";
static const char *THAI_LUNAR_WANING = "แรม";
static const char *THAI_LUNAR_KHAM   = "ค่ำ";
static const char *THAI_LUNAR_MONTH  = "เดือน";
static const char *THAI_LUNAR_LEAP   = "หลัง";

/* ---- Holidays ---- */
static const struct {
    const char *en;
    const char *th;
} THAI_HOLIDAY_MAP[] = {
    { "Magha Puja",                     "วันมาฆบูชา" },
    { "Visakha Puja",                   "วันวิสาขบูชา" },
    { "Attami Puja",                    "วันอัฏฐมีบูชา" },
    { "Asalha Puja",                    "วันอาสาฬหบูชา" },
    { "Khao Phansa",                    "วันเข้าพรรษา" },
    { "Ok Phansa",                      "วันออกพรรษา" },
    { "New Year",                       "วันขึ้นปีใหม่" },
    { "Chakri Day",                     "วันจักรี" },
    { "Songkran",                       "วันสงกรานต์" },
    { "Labour Day",                     "วันแรงงาน" },
    { "Coronation Day",                 "วันฉัตรมงคล" },
    { "Royal Plowing",                  "วันพืชมงคล" },
    { "Queen Suthida Birthday",         "วันเฉลิมฯ พระราชินี" },
    { "King Rama X Birthday",           "วันเฉลิมฯ ร.10" },
    { "Queen Mother Birthday",          "วันแม่" },
    { "King Rama IX Memorial",          "วันคล้ายวันสวรรคต ร.9" },
    { "Chulalongkorn Day",              "วันปิยมหาราช" },
    { "Father's Day",                   "วันพ่อ" },
    { "Constitution Day",               "วันรัฐธรรมนูญ" },
    { "New Year's Eve",                 "วันสิ้นปี" },
    { NULL, NULL }
};

static inline const char* thai_holiday_lookup(const char *en_name)
{
    if (!en_name) return NULL;
    size_t name_len = 0;
    while (en_name[name_len] && en_name[name_len] != '(' ) name_len++;
    while (name_len > 0 && en_name[name_len - 1] == ' ') name_len--;
    if (name_len == 0) return NULL;

    for (int i = 0; THAI_HOLIDAY_MAP[i].en; i++) {
        if (strlen(THAI_HOLIDAY_MAP[i].en) == name_len &&
            strncmp(THAI_HOLIDAY_MAP[i].en, en_name, name_len) == 0) {
            return THAI_HOLIDAY_MAP[i].th;
        }
    }
    return NULL;
}

/* ---- Misc UI strings ---- */
static const char *THAI_BE_PREFIX      = "พ.ศ.";
static const char *THAI_TODAY          = "วันนี้";
static const char *THAI_TOMORROW       = "พรุ่งนี้";
static const char *THAI_UPCOMING_HOL   = "วันหยุดที่จะมาถึง";
static const char *THAI_TODAY_SCHEDULE = "กำหนดการวันนี้";
static const char *THAI_WEATHER        = "สภาพอากาศ";
static const char *THAI_NEXT_ALARM     = "แจ้งเตือนถัดไป";
static const char *THAI_BANGKOK        = "กรุงเทพฯ";
static const char *THAI_FEELS_LIKE     = "รู้สึกเหมือน";
static const char *THAI_NTP_SYNCED     = "เวลาซิงค์แล้ว";
static const char *THAI_WAITING_NTP    = "รอซิงค์เวลา";
static const char *THAI_WAN_PHRA       = "วันพระ";

#ifdef __cplusplus
}
#endif
