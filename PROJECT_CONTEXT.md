# Digital Calendar Pi — Project Context

> **Single source of truth** for the Pi Zero W Digital Calendar project.
> Last updated: **May 8, 2026** (Phase 2.2.2 complete)

---

## 1. Project Goal

A wall-mounted Thai Buddhist calendar display running on Raspberry Pi Zero W with HDMI output. Ported and adapted from an existing ESP32 implementation (Panlee ZX7D00CE01S 7" display).

**End goal:** Always-on calendar showing today, lunar phase, holidays, weather, and timer — with Thai labels and a tulip-themed design.

**Reference UI:** ESP32 version (working) — see `D:\MY WORK\MY EMBEDED PROJECT\ESP32\digital_calendar\`

---

## 2. Hardware

| Item | Spec |
|---|---|
| Board | Raspberry Pi Zero WH |
| CPU | ARMv6, single-core 1 GHz |
| RAM | 512 MB |
| Storage | 64 GB SD card |
| Display | HDMI via mini-HDMI adapter, 1280x720 |
| OS | Raspberry Pi OS Lite Trixie 32-bit (kernel 6.12.75) |
| Hostname | `digitalcal-pi` |
| User | `thanat` |
| IP (DHCP) | `192.168.0.232` (LEnet WiFi) |
| PSU | 2A+ adapter (vcgencmd get_throttled = 0x0) |

---

## 3. Software Stack

```
Application layer
  └── Custom UI (theme, layout, mini_calendar)
LVGL 9.x (graphics library)
  ├── Drivers: FBDEV (display), EVDEV (input)
  └── Fonts: Montserrat 14/16/18/24/32/36 + thai_sarabun_24
Build system
  ├── CMake (template: lv_port_linux)
  ├── Make (-j1 for Pi Zero W resource constraints)
  └── tmux (for long-running builds)
OS
  └── Raspberry Pi OS Lite Trixie (Debian 13)
```

**Key file:** `lv_conf.defaults` controls which LVGL features are compiled. **DO NOT** strip aggressively — see Section 9 (Failed Experiments).

---

## 4. Folder Structure

```
D:\MY WORK\RASPBERRY PI PROJECT\         (Windows local, primary edit location)
├── CMakeLists.txt                       (433 lines — lv_port_linux template + our additions)
├── lv_conf.defaults                     (85 lines — LVGL config, KEEP AS-IS)
├── README.md                            (template README)
├── sync_to_pi.ps1                       (sync helper, scans + scp all source)
├── build_pi.ps1                         (build helper, tmux + cmake + make)
│
├── src/
│   ├── main.c                           (modified template entry point)
│   ├── main.cpp                         (LEGACY — orphan, not compiled)
│   ├── lv_conf.h                        (auto-generated, do not edit)
│   │
│   ├── ui/                              (our UI code — Phase 2.x)
│   │   ├── theme.h                      (color palette macros, theme_t struct)
│   │   ├── theme.cpp                    (DARK + LIGHT palettes, build_foundation_ui)
│   │   ├── layout.h                     (1280x720 native tokens, scaled 1.5x from ESP32)
│   │   ├── mini_calendar.h              (API: mini_calendar_build)
│   │   └── mini_calendar.cpp            (7x6 grid, today highlight, Thai labels)
│   │
│   ├── assets/                          (Phase 2.2.2)
│   │   ├── thai_sarabun_24.c            (124 KB, 3123 lines — LVGL font, supports v9)
│   │   ├── thai_fonts.h                 (LV_FONT_DECLARE wrapper)
│   │   └── thai_strings.h               (Thai weekday/month/zodiac/holiday strings)
│   │
│   └── lib/                             (lv_port_linux template helpers)
│       ├── driver_backends.c/h
│       ├── simulator_settings.h
│       ├── simulator_util.c/h
│       ├── mouse_cursor_icon.c
│       ├── display_backends/            (fbdev, drm, glfw3, sdl, wayland, x11)
│       └── indev_backends/              (evdev)
│
├── lvgl/                                (LVGL 9.x submodule, 500+ files)
└── build/                               (CMake output, not synced)

/home/thanat/digital_calendar_pi/        (Pi remote, mirrored from Windows)
└── (same structure, plus build/bin/lvglsim)
```

---

## 5. Phase Progress

| Phase | Status | Date | Notes |
|---|---|---|---|
| **2.1** Foundation | DONE | May 7 | Theme + Layout + first UI on HDMI, 30 FPS |
| **2.2.1** Mini Calendar skeleton | DONE | May 8 | 7x6 grid, today highlight, English labels, hardcoded May 2569 |
| **2.2.2** Thai Font + labels | DONE | May 8 | thai_sarabun_24 ported, Thai month/weekday strings |
| 2.2.2-OPT Strip lv_conf | REVERTED | May 8 | Black screen — see Section 9 |
| 2.2.3 Thai calendar logic | PENDING | — | Lunar, Buddhist year, holiday detection (port thai_calendar.cpp) |
| 2.2.4 Real time | PENDING | — | Linux time(), auto-refresh at midnight |
| 2.2.5 Tulip background | PENDING | — | bg_tulip.c image, translucent cards |
| 2.2.6 Header | PENDING | — | Weekday + date + clock + wanphra |
| 2.2.7 Weather card | PENDING | — | OpenWeather API, libcurl |
| 2.2.8 Upcoming holidays | PENDING | — | List card, sort by date |
| 2.2.9 Timer | PENDING | — | Buzzer + countdown |
| 2.2.10 Sidebar tabs | PENDING | — | Home / Month / Settings |

---

## 6. Working Features (Phase 2.2.2)

- Theme system — Dark navy palette, color macros (`C_BG_PRIMARY`, `C_ACCENT`, etc.)
- Layout tokens — 1280x720 native (scaled 1.5x from ESP32 800x480)
- Mini Calendar widget
  - Card with cyan border, navy background, radius 16
  - Title `"พฤษภาคม 2569"` (Thai month + Buddhist year)
  - Nav buttons `<` `🏠` `>` (visual only, no click handler)
  - DOW headers `อา จ อ พ พฤ ศ ส` (Thai short weekdays)
  - 7x6 day grid (1-31), Sun/Sat = pink (`#FF7A8C`), weekdays = white
  - Today (8) = cyan filled rounded box, dark navy text
  - Legend `[*] today [.] event [.] holiday`
- Performance: 30 FPS, CPU 4%, render 2 ms — Pi Zero W has plenty of headroom

---

## 7. Build & Run Workflow

### From Windows (primary):

```powershell
cd "D:\MY WORK\RASPBERRY PI PROJECT"

# After editing source files:
.\sync_to_pi.ps1                  # Push all source files to Pi (~28 files)
.\build_pi.ps1                    # Incremental build in tmux
.\build_pi.ps1 -Reconfigure       # Full rebuild (when CMakeLists.txt or lv_conf.defaults changes)

# Detach from tmux: Ctrl+B then D
# Reattach: ssh thanat@192.168.0.232 -t "tmux attach -t build"
```

### From Pi (after build):

```bash
ssh thanat@192.168.0.232    # Login
cal                          # Run lvglsim (alias)
# Ctrl+C to stop
calhealth                    # PSU + temp + uptime
bye                          # sudo shutdown -h now
```

### Pi-side aliases (in ~/.bashrc):

```bash
alias cal='cd ~/digital_calendar_pi/build && ./bin/lvglsim'
alias bye='sudo shutdown -h now'
alias calhealth='vcgencmd get_throttled; vcgencmd measure_temp; uptime'
```

---

## 8. Build Time Reality

**Important:** "Incremental build is fast" is true in theory, but our workflow rarely hits that path because we frequently touch `CMakeLists.txt` or `lv_conf.defaults`.

| Scenario | Build time |
|---|---|
| Fresh / clean build | 30–60 min |
| `lv_conf.defaults` changed | 25–40 min (full LVGL recompile) |
| `CMakeLists.txt` changed (add new source file) | 25–40 min |
| Source-only edit (`.cpp/.h/.c`) | 30 sec – 2 min ✅ |

**Implication:** Phase 2.2.x phases that add new source files (most of them) trigger full rebuild. This is why a Desktop simulator (build in 2–5 sec) is on the roadmap.

---

## 9. Failed Experiments / Lessons

### Phase 2.2.2-OPT (May 8) — Aggressive lv_conf strip

**What was tried:** Disable ~27 widgets, 16 fonts, ThorVG, Lottie, image decoders, and most drivers in `lv_conf.defaults` to cut build time by ~60%.

**What happened:** Build succeeded but **HDMI showed black screen**. Lek caught it immediately and reverted.

**Root cause (suspected):** Disabled some flag that LVGL's internal rendering path depends on (likely a theme, font, or display flag). Not investigated further — reverted to working state.

**Lesson:** "Audit then disable" is the right principle, but disabling many things at once makes failure unattributable. Future optimization should:
1. Disable in small batches (e.g., 5 fonts at a time)
2. Build + verify after each batch
3. Have a Desktop simulator ready so each verify cycle is 2 sec, not 25 min

**Decision:** Do not re-attempt lv_conf optimization until Desktop simulator is set up.

### Other lessons captured

- **CMake "smart caching" doesn't help us** — every phase touches a tripwire. Practical incremental builds are rare in current workflow.
- **scp one file at a time → easy to forget** → use `sync_to_pi.ps1` instead.
- **`lv_obj_clear_flag(obj, FLAG_A | FLAG_B)`** — LVGL 9 rejects OR'd flags. Split into two calls.
- **`snprintf` requires `#include <cstdio>`** in LVGL 9 (stricter than 8.4).
- **Pi Zero W rejects new SSH connections during heavy compile** — always use tmux for long builds.
- **Mouse scroll inside tmux** prints ASCII garbage but does not affect the build.

---

## 10. Pending / Known Issues

- **`src/main.cpp`** is an orphan from an earlier mistake. Not compiled (only `main.c` is in `add_executable`). Safe to delete eventually.
- **Subtitle "Phase 2.2.2"** is hardcoded in `theme.cpp` — must update each phase manually.
- **Today = hardcoded 8** in `mini_calendar.cpp` — fixes in Phase 2.2.4 (real time).
- **DOW for May 1 = hardcoded 4 (Thursday)** — fixes in Phase 2.2.3 (Thai calendar logic).
- **No Git repo yet for Pi project** — recommended before Phase 2.2.3 to enable safe revert. ESP32 repo: `thanatvipulakorn-arch/digital_calendar` (master).
- **No Desktop simulator yet** — priority before Phase 2.2.5+ to make UI iteration practical.

---

## 11. Reference: ESP32 Source

When porting features, consult ESP32 source:

```
D:\MY WORK\MY EMBEDED PROJECT\ESP32\digital_calendar\
├── ui_home.cpp                  (1297 lines — main UI, mini cal, header, cards)
├── ui_layout.h                  (800x480 tokens — scale 1.5x for our 1280x720)
├── ui_thai_strings.h            (Thai labels — already ported to assets/thai_strings.h)
├── ui_theme.cpp                 (theme switcher, 8 panel presets)
├── thai_calendar.cpp/h          (lunar + Buddhist year + holidays — port in 2.2.3)
├── thai_sarabun_24.c            (font — already ported to assets/)
├── thai_sarabun_stacked_24.c    (vowel-stacked font — port in 2.2.6 for header)
├── ui_charts.cpp                (~600 lines — for Phase 2.3+ Month view)
├── ui_settings.cpp              (~700 lines — for Phase 2.3+ Settings tab)
└── ... (web server, WiFi, NVS — Pi uses different APIs)
```

**Key API differences ESP32 → Pi:**

| ESP32 | Pi (Linux) |
|---|---|
| `Arduino.h`, `time_t` from RTC | `<time.h>`, system time |
| FFat filesystem | Standard Linux fs |
| `WiFi.h`, NVS | systemd-networkd, file config |
| FreeRTOS timer | pthread / LVGL timer |
| ESP_IO_Expander, GT911 touch | EVDEV (mouse/keyboard) |
| LVGL 8.4 | LVGL 9.x |

---

## 12. Next Session Checklist

When resuming work:

1. **Verify still working:** `ssh thanat@192.168.0.232` → `cal` → see Thai mini calendar on HDMI.
2. **Check Pi storage:** `df -h /` (need >2 GB free for builds).
3. **Pull latest from Windows:** `.\sync_to_pi.ps1` (covers any out-of-date file).
4. **Decide next phase:**
   - **2.2.3** if continuing UI features (faster ROI)
   - **WSL2 + Desktop simulator setup** if planning many UI iterations (better long-term)
5. **Open this file** for reference.

---

## 13. Key Decisions Log

- **2026-05-06** — Pi Zero W chosen as production target (vs Pi 4/5) because use case = static wall calendar.
- **2026-05-06** — `lv_port_linux` template chosen over `lv_port_pc_eclipse` to avoid ARMv6 Helium ASM issue with LVGL 9.
- **2026-05-07** — Resolution set to 1280x720 (HDMI native), layout tokens scaled 1.5x from ESP32's 800x480.
- **2026-05-07** — File transfer via scp + PowerShell scripts (not VS Code SFTP — unreliable for new files).
- **2026-05-08** — Thai font kept at 24px only (vs ESP32's 14/24/stacked) to limit assets folder size.
- **2026-05-08** — Aggressive lv_conf optimization deferred until Desktop simulator is available.
- **2026-05-08** — Desktop simulator (WSL2 + LVGL PC port) is highest-ROI next infrastructure investment.
