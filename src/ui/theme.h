/* ================================================================
 * theme.h - Color palette for Digital Calendar Pi
 *
 * Ported from ESP32 ui_theme.h (Phase 2.1)
 *
 * Differences from ESP32:
 *   - No NVS (no persistent storage yet)
 *   - No auto-switch (Dark only for now)
 *   - No 8 panel presets (single Dark theme)
 *
 * Future:
 *   - Light theme support (Phase 2.5)
 *   - Theme persistence (Phase 3)
 * ================================================================ */
#pragma once

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ──────────── Theme Modes ──────────── */
typedef enum {
    THEME_MODE_DARK  = 0,
    THEME_MODE_LIGHT = 1,
} theme_mode_t;

/* ──────────── Theme Struct ──────────── */
typedef struct {
    /* Backgrounds */
    lv_color_t bg_primary;     /* Root background (navy) */
    lv_color_t bg_secondary;   /* Card background */
    lv_color_t bg_elevated;    /* Hover/highlight */

    /* Accents */
    lv_color_t accent;         /* Primary accent (cyan) */
    lv_color_t success;        /* Green / holiday */
    lv_color_t warning;        /* Amber / Buddhist day */
    lv_color_t error;          /* Red / weekend */

    /* Text */
    lv_color_t text_primary;   /* Main text */
    lv_color_t text_muted;     /* Secondary text */
    lv_color_t text_hint;      /* Hints / subtitles */

    /* Borders */
    lv_color_t border;
} theme_t;

/* ──────────── Active theme ──────────── */
extern theme_t g_theme;

/* ──────────── Color Macros (back-compat with ESP32 code) ──────────── */
#define C_BG_PRIMARY       g_theme.bg_primary
#define C_BG_SECONDARY     g_theme.bg_secondary
#define C_BG_ELEVATED      g_theme.bg_elevated

#define C_ACCENT           g_theme.accent
#define C_SUCCESS          g_theme.success
#define C_WARNING          g_theme.warning
#define C_ERROR            g_theme.error

#define C_TEXT_PRIMARY     g_theme.text_primary
#define C_TEXT_MUTED       g_theme.text_muted
#define C_TEXT_HINT        g_theme.text_hint

#define C_BORDER           g_theme.border

/* ──────────── Theme API ──────────── */

/**
 * Initialize active theme.
 * Must be called once in main() BEFORE building any UI.
 */
void theme_init(theme_mode_t mode);

/**
 * Build foundation UI (Phase 2.1 - skeleton).
 * Shows theme background + status card.
 * Call AFTER theme_init() and AFTER backend init.
 */
void build_foundation_ui(void);

#ifdef __cplusplus
}
#endif
