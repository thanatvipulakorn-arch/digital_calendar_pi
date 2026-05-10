/*******************************************************************
 *
 * main.c - LVGL simulator for GNU/Linux
 *
 * Based on the original file from the repository
 *
 * @note eventually this file won't contain a main function and will
 * become a library supporting all major operating systems
 *
 * To see how each driver is initialized check the
 * 'src/lib/display_backends' directory
 *
 * - Clean up
 * - Support for multiple backends at once
 *   2025 EDGEMTech Ltd.
 *
 * Author: EDGEMTech Ltd, Erik Tagirov (erik.tagirov@edgemtech.ch)
 *
 ******************************************************************/
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lvgl/lvgl.h"

#include "src/lib/driver_backends.h"
#include "src/lib/simulator_util.h"
#include "src/lib/simulator_settings.h"

/* Digital Calendar Pi - foundation UI */
#include "ui/theme.h"
#include "ui/layout.h"
#include "ui/header.h"

/* Phase 2.2.3 - Thai lunar calendar logic (port from ESP32) */
#include "calendar/thai_calendar.h"

/* Internal functions */
static void configure_simulator(int argc, char ** argv);
static void print_lvgl_version(void);
static void print_usage(void);
void header_tick_trampoline(lv_timer_t *t);   /* Phase 2.2.6 — see bottom of file */

/* contains the name of the selected backend if user
 * has specified one on the command line */
static char * selected_backend;

/* Global simulator settings, defined in lv_linux_backend.c */
extern simulator_settings_t settings;

/* Manual redraw escape hatch (post-2.2.5x).
 * When agetty re-renders /etc/issue (e.g. on every IP change while
 * WiFi is failing), it overdraws our framebuffer. LVGL won't repaint
 * those pixels because nothing in the widget tree is dirty.
 * Sending SIGUSR1 sets a flag; the lv_timer below picks it up on the
 * LVGL thread and invalidates the active screen, forcing a repaint.
 * (Signal handlers must not call LVGL APIs directly — not thread-safe
 * and not async-signal-safe.) */
static volatile sig_atomic_t need_redraw = 0;

static void handle_redraw_signal(int sig)
{
    (void)sig;
    need_redraw = 1;
}

static void redraw_check_cb(lv_timer_t *t)
{
    (void)t;
    if (need_redraw) {
        need_redraw = 0;
        lv_obj_t *scr = lv_screen_active();
        if (scr) {
            lv_obj_invalidate(scr);
        }
    }
}


/**
 * @brief Print LVGL version
 */
static void print_lvgl_version(void)
{
    fprintf(stdout, "%d.%d.%d-%s\n",
            LVGL_VERSION_MAJOR,
            LVGL_VERSION_MINOR,
            LVGL_VERSION_PATCH,
            LVGL_VERSION_INFO);
}

/**
 * @brief Print usage information
 */
static void print_usage(void)
{
    fprintf(stdout,
            "\nlvglsim [-V] [-B] [-f] [-m] [-b backend_name] [-W window_width] [-H window_height] [-R rotation]\n\n");
    fprintf(stdout, "-V print LVGL version\n");
    fprintf(stdout, "-B list supported backends\n");
    fprintf(stdout, "-f fullscreen\n");
    fprintf(stdout, "-m maximize\n");
}

/**
 * @brief Configure simulator
 * @description process arguments received by the program to select
 * appropriate options
 * @param argc the count of arguments in argv
 * @param argv The arguments
 */
static void configure_simulator(int argc, char ** argv)
{
    int opt = 0;

    selected_backend = NULL;
    driver_backends_register();

    const char * env_w = getenv("LV_SIM_WINDOW_WIDTH");
    const char * env_h = getenv("LV_SIM_WINDOW_HEIGHT");
    /* Default values — 1920x1080 to match the Samsung HDMI native fb size.
     * Override via env LV_SIM_WINDOW_WIDTH / LV_SIM_WINDOW_HEIGHT or -W/-H. */
    settings.window_width = atoi(env_w ? env_w : "1920");
    settings.window_height = atoi(env_h ? env_h : "1080");

    /* Parse the command-line options. */
    while((opt = getopt(argc, argv, "b:fmW:H:R:BVh")) != -1) {
        switch(opt) {
            case 'h':
                print_usage();
                exit(EXIT_SUCCESS);
                break;
            case 'V':
                print_lvgl_version();
                exit(EXIT_SUCCESS);
                break;
            case 'B':
                driver_backends_print_supported();
                exit(EXIT_SUCCESS);
                break;
            case 'b':
                if(driver_backends_is_supported(optarg) == 0) {
                    die("error no such backend: %s\n", optarg);
                }
                selected_backend = strdup(optarg);
                break;
            case 'f':
                settings.fullscreen = true;
                break;
            case 'm':
                settings.maximize = true;
                break;
            case 'W':
                settings.window_width = atoi(optarg);
                break;
            case 'H':
                settings.window_height = atoi(optarg);
                break;
            case 'R':
                switch(atoi(optarg)) {
                    case 0:
                        settings.rotation = LV_DISPLAY_ROTATION_0;
                        break;
                    case 90:
                        settings.rotation = LV_DISPLAY_ROTATION_90;
                        break;
                    case 180:
                        settings.rotation = LV_DISPLAY_ROTATION_180;
                        break;
                    case 270:
                        settings.rotation = LV_DISPLAY_ROTATION_270;
                        break;
                    default:
                        LV_LOG_WARN("Invalid rotation angle. Valid angles are {0, 90, 180, 270}");
                        break;
                }
                break;
            case ':':
                print_usage();
                die("Option -%c requires an argument.\n", optopt);
                break;
            case '?':
                print_usage();
                die("Unknown option -%c.\n", optopt);
        }
    }
}

/**
 * @brief entry point
 * @description start a demo
 * @param argc the count of arguments in argv
 * @param argv The arguments
 */
int main(int argc, char ** argv)
{

    configure_simulator(argc, argv);

    /* ----------------------------------------------------------------
     * Phase 2.2.3 — thai_calendar self-test
     *   Prints lunar dates for a few known reference points so we can
     *   eyeball-verify the port matches the ESP32 output and myhora.com.
     *   Remove or guard once UI integration (2.2.4+) is done.
     * ---------------------------------------------------------------- */
    {
        struct {
            int y, m, d;
            const char *note;
        } samples[] = {
            { 2026,  5,  8, "today (May 8, 2026)" },
            { 2026,  3, 19, "m5_day1 boundary 2569 (lunar new year)" },
            { 2026,  3, 18, "day before m5_day1 2569 (still 2568 territory)" },
            { 2026,  6, 16, "Visakha Puja (leap year shifts to month 7)" },
            { 2027,  4,  7, "m5_day1 boundary 2570" },
            { 2025,  5, 12, "Visakha Puja 2568 (normal year, month 6)" },
        };
        const int n = (int)(sizeof(samples) / sizeof(samples[0]));
        fprintf(stderr, "\n=== thai_calendar self-test ===\n");
        for (int i = 0; i < n; i++) {
            thai_lunar_t lu = thai_calendar_from_date(samples[i].y,
                                                      samples[i].m,
                                                      samples[i].d);
            char en[80];
            thai_calendar_format_summary_en(&lu, en, sizeof(en));
            const char *hol = thai_calendar_holiday_en(&lu);
            const char *fix = thai_calendar_fixed_holiday_en(samples[i].m,
                                                             samples[i].d);
            fprintf(stderr,
                    "  %04d-%02d-%02d  %-50s  buddhist=%d  holiday=%s  fixed=%s\n",
                    samples[i].y, samples[i].m, samples[i].d, en,
                    thai_calendar_is_buddhist_day(&lu),
                    hol ? hol : "-",
                    fix ? fix : "-");
            (void)samples[i].note;
        }
        fprintf(stderr, "=== end self-test ===\n\n");
    }

    /* Initialize LVGL. */
    lv_init();

    /* Initialize the configured backend */
    if(driver_backends_init_backend(selected_backend) == -1) {
        die("Failed to initialize display backend");
    }
    if(settings.rotation) {
#if LV_USE_DRAW_NANOVG && LV_DRAW_TRANSFORM_USE_MATRIX
        lv_display_set_matrix_rotation(NULL, true);
#endif
        lv_display_set_rotation(NULL, settings.rotation);
    }

    /* Enable for EVDEV support */
#if LV_USE_EVDEV
    if(driver_backends_init_backend("EVDEV") == -1) {
        die("Failed to initialize evdev");
    }
#endif

    /* Build Digital Calendar UI */
    theme_init(THEME_MODE_DARK);
    build_foundation_ui();

    /* Phase 2.2.6: 1 Hz tick refreshes the header (clock per-second,
     * date/lunar/wanphra on midnight rollover — see header_tick()).
     * Cheap: header_tick early-returns when nothing changed. */
    lv_timer_create(header_tick_trampoline, 1000, NULL);

    /* SIGUSR1 -> screen invalidate. Manual escape hatch for the
     * agetty/console-overdraw bug (see comment near handle_redraw_signal).
     * Trigger: `kill -USR1 $(pgrep lvglsim)` or `calredraw` alias. */
    signal(SIGUSR1, handle_redraw_signal);
    lv_timer_create(redraw_check_cb, 100, NULL);

    /* Enter the run loop of the selected backend */
    driver_backends_run_loop();

    return 0;
}

/* Trampoline so the lv_timer_cb_t signature matches header_tick(void) */
void header_tick_trampoline(lv_timer_t *t)
{
    (void)t;
    header_tick();
}
