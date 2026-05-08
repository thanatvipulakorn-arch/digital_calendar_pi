#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

#define LV_USE_STDLIB_MALLOC    LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING    LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_CLIB

#define LV_DEF_REFR_PERIOD  33
#define LV_DPI_DEF          130

#define LV_DRAW_BUF_STRIDE_ALIGN                1
#define LV_DRAW_BUF_ALIGN                       4
#define LV_DRAW_SW_SUPPORT_RGB565               1
#define LV_DRAW_SW_SUPPORT_RGB888               1
#define LV_DRAW_SW_SUPPORT_ARGB8888             1

/* Disable assembly optimizations — Pi Zero W (ARMv6) doesn't support
   Helium (Cortex-M55+) or Neon SIMD instructions */
#define LV_USE_DRAW_SW_ASM                      LV_DRAW_SW_ASM_NONE

#define LV_USE_OS   LV_OS_NONE

#define LV_USE_LOG  1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

#define LV_USE_SYSMON               1
#define LV_USE_PERF_MONITOR         1
#define LV_USE_PERF_MONITOR_POS     LV_ALIGN_BOTTOM_RIGHT

#define LV_FONT_MONTSERRAT_14    1
#define LV_FONT_MONTSERRAT_24    1
#define LV_FONT_MONTSERRAT_32    1

#define LV_FONT_DEFAULT &lv_font_montserrat_14

#define LV_USE_LABEL        1
#define LV_USE_BUTTON       1
#define LV_USE_OBJ          1

#define LV_USE_LINUX_FBDEV     1
#define LV_LINUX_FBDEV_BSD     0

#endif
