/* ================================================================
 * layout.h - Layout tokens for Digital Calendar Pi (1280x720)
 *
 * Ported from ESP32 ui_layout.h (Phase 2.1)
 *
 * Philosophy: parametric math-based layout, not hard-coded pixels.
 * All component sizes are derived from a few base constants.
 *
 * Screen: 1280 × 720 (HDMI native)
 *
 * Scale ratio from ESP32 (800x480):
 *   Width:  1280 / 800 = 1.6x
 *   Height: 720  / 480 = 1.5x
 *
 * Note: Pi version has NO sidebar/topbar (single-screen focused).
 *       Direct full-screen calendar layout.
 * ================================================================ */
#pragma once

#include "theme.h"

/* ──────────── Screen dimensions ──────────── */
#define SCREEN_W           1280
#define SCREEN_H           720

/* ──────────── Spacing tokens (scaled 1.5x from ESP32) ──────────── */
#define GAP_XS             6     /* was 4 on ESP32 */
#define GAP_SM             12    /* was 8  */
#define GAP_MD             18    /* was 12 */
#define GAP_LG             24    /* was 16 */

/* ──────────── Padding ──────────── */
#define PADDING            16    /* was 10 */
#define CARD_RADIUS        16    /* was 12 */

/* ──────────── Safe area (full-screen, no sidebar) ──────────── */
#define HOME_W             (SCREEN_W - 2 * PADDING)   /* 1248 */
#define HOME_H             (SCREEN_H - 2 * PADDING)   /* 688 */

/* ──────────── Header (top section) ──────────── */
#define HDR_H              90    /* was 60, scaled 1.5x */
#define HDR_GAP            GAP_LG

/* ──────────── Main grid (below header) ──────────── */
#define GRID_Y             (HDR_H + HDR_GAP)
#define GRID_H             (HOME_H - GRID_Y)

/* ──────────── Card opacity (for translucent cards) ──────────── */
/* 153 = 60% opaque (from ESP32 — for tulip background later) */
#define CARD_OPA_TRANSLUCENT  153
