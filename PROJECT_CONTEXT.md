# Digital Calendar Pi — Project Context

> **Single source of truth** for the Pi Zero W Digital Calendar project.
> Last updated: **May 9, 2026** — Phase 2.2.5x complete, project shipped (v1.0.0).

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
| IP (DHCP) | `192.168.0.232` (LEnet WiFi @ office) / `192.168.1.107` (eth0 USB adapter @ home) / `192.168.1.106` (wlan0 @ home) |
| PSU | 2A+ adapter (vcgencmd get_throttled = 0x0) |
| USB Ethernet | USB-OTG → RJ45 adapter (workaround for home WiFi issues — see §9) |

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
├── .claude/
│   └── skills/
│       └── pi-lvgl-build-workflow/
│           └── SKILL.md                 (Phase 2.2.5d — captured workflow gotchas
│                                         for future AI sessions: ccache rule, sync
│                                         v2, network failure tree, LVGL 9 idioms,
│                                         Thai stacked-font rule)
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
│   │   ├── mini_calendar.cpp            (7x6 grid, today highlight, Thai labels)
│   │   ├── header.h / header.cpp        (Phase 2.2.6 — top bar: weekday/date/lunar/clock/wanphra)
│   │   ├── upcoming.h / upcoming.cpp    (Phase 2.2.8 — next holidays card)
│   │   └── timer_card.h / timer_card.cpp (Phase 2.2.9 — visual placeholder)
│   │
│   ├── calendar/                        (Phase 2.2.3 — Thai lunar math)
│   │   ├── thai_calendar.h              (5.3 KB — public API, lunar/zodiac types)
│   │   └── thai_calendar.cpp            (16.1 KB, 454 lines — pure math, ported verbatim from ESP32)
│   │
│   ├── assets/                          (Phase 2.2.2)
│   │   ├── thai_sarabun_24.c            (124 KB, 3123 lines — LVGL font, supports v9)
│   │   ├── thai_fonts.h                 (LV_FONT_DECLARE wrapper)
│   │   ├── thai_strings.h               (Thai weekday/month/zodiac/holiday strings)
│   │   ├── bg_tulip.c                   (Phase 2.2.5 — 4.49 MB, 800x480 RGB565 tulip image)
│   │   └── bg_tulip.h                   (LV_IMAGE_DECLARE for bg_tulip)
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
| **2.2.3** Thai calendar logic | DONE | May 9 | Ported `thai_calendar.{h,cpp}` verbatim from ESP32. Self-test on Pi verified all 6 reference dates match myhora.com (lunar/zodiac/leap year). Git initialised. |
| **2.2.4** Real time | DONE | May 9 | `mini_calendar.cpp` now reads `time(NULL)` + `localtime_r()` for today/DOW/days/title. Subtitle in `theme.cpp` updated to "Phase 2.2.4 - Real Time". Visual diff verified on HDMI (today highlight moved from hardcoded 8 → 9). Midnight auto-refresh deferred. |
| **2.2.5** Tulip background | DONE | May 9 | bg_tulip.c image (4.49 MB, 800x480 RGB565 ported from ESP32), full-screen via `LV_IMAGE_ALIGN_STRETCH`. mini_calendar/upcoming/timer cards translucent (opa 220). |
| **2.2.5b** Native-res wallpaper | DONE | May 9 | Calendarbackground_2.jpg → bg_calendar.c (1920x1080 RGB565, 24.5 MB source) — replaces upscaled bg_tulip. PowerShell + System.Drawing + LockBits + inline C# (~2 sec generate). 1920x1080 layout with EDGE_MARGIN=60 to dodge TV overscan. |
| **2.2.5c** Clock zoomed | DONE | May 9 | `header.cpp` clock label uses `transform_scale_x=460/y=384` (1.8x × 1.5x) to clear the Montserrat 48 builtin ceiling. Asymmetric per Lek's spec — wider than tall. |
| **2.2.5d** Layout overhaul + stacked Thai font | DONE | May 9 | Mini calendar 1100×720 → **1368×832** (left edge to x=24, bottom matches right column). Day numbers font 36 → **48** (max builtin). Per-cell **grid lines** (right+bottom border, share with neighbours). DOW headers **2.2x transform_scale** so they're larger than day numbers. Timer card **removed permanently** (state machine + buzzer deferred indefinitely). **Weather card stub** added at top right (libcurl wiring is Phase 2.2.7-NET). All UI labels swept from `thai_sarabun_24` → `thai_sarabun_stacked_24` to fix tone-mark stacking on words like ที่/นี้/ขึ้น. **ccache** installed on Pi and wired in CMakeLists. **CMAKE_BUILD_TYPE** pinned in CMakeLists. |
| **2.2.5e** Degree-symbol tofu + headline | DONE | May 9 | Weather "°C" was rendering as ☐ — Thai font's range is 32-127 + 3584-3711, no Latin-1. Split the temp unit and feels-like meta into Thai-font + Montserrat-font label pairs. Upcoming title bumped to C_ACCENT (cyan) + 1.4x scale. |
| **2.2.5f** DOW colours, size, alignment | DONE | May 9 | Per-day Thai colour palette: อา/ส = red (0xFF7A8C, kept); จ = yellow; อ = pink; พ = green; พฤ = orange; ศ = blue. DOW scale 2.2x → 2.8x; cell height 80 → 100; aligned TOP_MID (offset 18) instead of centre. |
| **2.2.5g** Upcoming row spacing | DONE | May 9 | row_h 38 → 56, row_y0 44 → 80 (clearance for scaled headline + breathing room). |
| **2.2.5h** Card translucency + grid line visibility | DONE | May 9 | All cards bg_opa 220 → 170 (~67%); header bar 200 → 160. Grid lines 0x506070/LV_OPA_50 → 0xE0EFFF/200 (light blue-white at 78%). |
| **2.2.5i** Explicit grid lines | DONE | May 9 | Per-cell right+bottom borders had visible gaps; replaced with first-class lv_obj 1-px hline/vline objects for pixel-perfect continuity. |
| **2.2.5j** Closed grid + DOW lift + holiday markers | DONE | May 9 | Added left/right/top edge lines to close the table border. DOW lifted toward visual centre with `LV_ALIGN_CENTER, 0, -25` (still wrong — see 2.2.5n). Holiday/wanphra markers introduced: amber tint + Thai name for national holidays, yellow text + circle for regular วันพระ. |
| **2.2.5k** Holiday colour swap | DONE | May 9 | Holiday tint amber → purple (`#BA68C8`) so it doesn't look like wanphra yellow. Number text on tinted cells → light lilac (`#E1BEE7`) for contrast. Royal-Thai colour vibe. |
| **2.2.5l** Stub label cleanup + Visakha wanphra | DONE | May 9 | Removed `(stub - Phase 2.2.7-NET)` from weather card; relaxed `is_wanphra && !is_holiday` to `is_wanphra && !is_today` so May 31 (Visakha = wanphra + holiday) also gets the yellow circle (positioned above the holiday name label). |
| **2.2.5n** DOW transform_pivot fix | DONE | May 9 | Lek's diagnosis: day numbers (no transform) centred fine with lv_obj_center; DOW (transform_scale 2.8x) drifted because LVGL's default pivot is top-left. Pinned `transform_pivot_x/y = lv_pct(50)` and dropped the y-offset hack — DOW now centres perfectly with the same `lv_obj_center` as the day numbers. **The big take-away of the day:** the y-offset hacks were treating the symptom; the real bug was the default transform pivot. Captured in SKILL.md. |
| **2.2.5p** Production UI cleanup | DONE | May 9 | Removed bottom-left "Phase 2.2.x" dev marker, mini-cal legend, and weather "(stub)" hint. UI now reads as a finished product. |
| **2.2.5q** Wanphra circle on today | DONE | May 9 | Today + wanphra (May 9) was missing the yellow circle because of the `!is_today` guard; relaxed to always show the circle on wanphra. Yellow on cyan is fine visually. |
| **2.2.5r** WiFi label visibility | DONE | May 9 | Slate-grey 14 px was invisible on translucent navy + tulip; bumped to white / 18 px and moved from y=64 to y=90 below the scaled clock visual. |
| **2.2.5s** WiFi indicator + clock centring | DONE | May 9 | Same `transform_pivot 50/50` fix from 2.2.5n applied to the clock. WiFi indicator now uses `lv_label_set_recolor` with inline `#RRGGBB ...#` so `LV_SYMBOL_WIFI` is green when associated, red when offline. SSID query switched from iwgetid (not on Trixie) to nmcli. |
| **2.2.6** Header | DONE | May 9 | `header.{h,cpp}` — weekday/date/lunar/clock/wanphra. 1 Hz lv_timer in main.c → `header_tick()` updates clock per-second, others on day_changed. |
| 2.2.7 Weather card (NET) | PENDING (Batch B) | — | Replace stub with libcurl + cJSON Open-Meteo client + 5-min refresh on lv_timer in worker thread |
| **2.2.8** Upcoming holidays | DONE | May 9 | `upcoming.{h,cpp}` — scans 90 days via `thai_calendar_*`, sorts ascending, shows up to 5 with offset chip. Snapshot at build (no midnight refresh yet). |
| 2.2.9 Timer | REMOVED | May 9 | Card removed permanently; will revisit if/when GPIO + audio HAT is decided. |
| 2.2.10 Sidebar tabs | PENDING (Batch C) | — | Home / Month / Settings — port `ui_charts.cpp` + `ui_settings.cpp` |
| 2.2.5 Tulip background | PENDING | — | bg_tulip.c image, translucent cards |
| 2.2.6 Header | PENDING | — | Weekday + date + clock + wanphra |
| 2.2.7 Weather card | PENDING | — | OpenWeather API, libcurl |
| 2.2.8 Upcoming holidays | PENDING | — | List card, sort by date |
| 2.2.9 Timer | PENDING | — | Buzzer + countdown |
| 2.2.10 Sidebar tabs | PENDING | — | Home / Month / Settings |

---

## 6. Working Features (Phase 2.2.3)

- Theme system — Dark navy palette, color macros (`C_BG_PRIMARY`, `C_ACCENT`, etc.)
- Layout tokens — 1280x720 native (scaled 1.5x from ESP32 800x480)
- Mini Calendar widget
  - Card with cyan border, navy background, radius 16
  - Title `"พฤษภาคม 2569"` (Thai month + Buddhist year — currently hardcoded; 2.2.4 makes real)
  - Nav buttons `<` `🏠` `>` (visual only, no click handler)
  - DOW headers `อา จ อ พ พฤ ศ ส` (Thai short weekdays)
  - 7x6 day grid (1-31), Sun/Sat = pink (`#FF7A8C`), weekdays = white
  - Today (8) = cyan filled rounded box, dark navy text
  - Legend `[*] today [.] event [.] holiday`
- **Thai lunar calendar logic** (Phase 2.2.3) — pure math layer, not yet wired to UI
  - `thai_calendar_from_date(y, m, d)` → `thai_lunar_t {waxing, day, month, leap_month, year_ce, zodiac_idx, valid}`
  - `thai_calendar_is_buddhist_day()` — waxing 8/15, waning 8, end-of-month
  - `thai_calendar_holiday_en()` — Magha/Visakha/Asalha/Khao Phansa/Ok Phansa (with leap-year shifts)
  - `thai_calendar_fixed_holiday_en()` — Gregorian-fixed Thai holidays (Songkran, Chakri, etc.)
  - `thai_calendar_zodiac_th/en()` — 12-year cycle from year_ce
  - Coverage: 2025–2031 (m5_day1 epochs verified vs myhora.com)
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

After Phase 2.2.5d adopted **ccache** + **sync_to_pi.ps1 v2** (checksum-aware) + **CMAKE_BUILD_TYPE pinned in CMakeLists**, real-world Pi Zero W build cycles look like this:

| Scenario | Time | Why |
|---|---|---|
| First build ever (cache empty) | ~70 min | LVGL ~600 files compile + ccache populates |
| `lv_conf.defaults` changed | ~50 min | LVGL config invalidates ALL `.o` (ccache cannot help — different flags) |
| `CMakeLists.txt` changed + ccache populated | **~3-5 min** ⚡ | CMake regenerates, but ccache hits replay LVGL `.o` instantly |
| Source-only edit (`.cpp/.h/.c`) | **20-60 sec** ⚡ | sync v2 only copies the changed file, make recompiles 1 `.o` + relink |
| `cmake ..` without `-DCMAKE_BUILD_TYPE=Release` | ~70 min | Build type defaults to empty/Debug → all `.o` invalidated. **Don't do this.** Pin in CMakeLists instead (already done in this project as of 2.2.5d). |

**Rule of thumb:** if "incremental" feels longer than ~5 min, *something is invalidating cache*. Investigate immediately — don't accept it as "Pi Zero W is just slow".

**Pre-2.2.5d numbers (kept for context):** before ccache + sync v2, every CMakeLists.txt edit took ~70 min and even pure source edits took ~30 min because sync v1 bumped mtime of CMakeLists/lv_conf.defaults → CMake regenerated lv_conf.h → cascade. The 200x speed-up (60 min → 18 sec on a no-op rebuild) was verified end-to-end on May 9.

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

### Phase 2.2.3 lessons (May 9) — Home WiFi network debugging

The Phase 2.2.3 build attempt at home consumed ~1 hour of network debugging
before a workable path was found. Captured here so future sessions skip it.

**Symptoms:** ssh from Windows → Pi via WiFi failed at "banner exchange:
Connection to UNKNOWN port -1: Connection refused". Verbose ssh showed
TCP "Connection established" + "getpeername failed" — three-way handshake
appeared to complete from Windows kernel's view, but Pi's sshd never
logged any connection from 192.168.1.104.

**Layers that contributed (not all of them mattered, but each took time
to rule out):**

1. ProtonVPN active on Windows — TUN adapter, killswitch firewall
2. Manual `route add 192.168.1.0 mask 255.255.255.0 192.168.1.1 metric 1`
   (suggested while debugging VPN) — overrode the on-link route, sent LAN
   traffic via gateway, broke direct delivery
3. `route delete 192.168.1.0` removed not just the manual route but the
   auto-created on-link route too — Windows ended up sending intra-LAN
   traffic via the default gateway → AP isolation kicked in
4. Pi sshd was disabled by default on Trixie image — needed
   `sudo raspi-config nonint do_ssh 0`
5. Windows network profile of `buabeam_2.4G` was Public (default for new
   WiFi) — blocked inbound from LAN even after profile fix
6. **Final root cause: home Wi-Fi had AP/Client Isolation or some
   wireless-layer interference that survived all of the above.** The same
   `buabeam_2.4G` SSID worked fine for ESP32 in earlier projects, so the
   block is layered (TCP-specific, not ICMP).

**What actually worked:** USB OTG → RJ45 adapter on the Pi, plug Ethernet
into the home router. Pi got a separate IP on `eth0` (192.168.1.107),
ssh worked first try. Wireless layer fully bypassed.

**Decisions captured:**
- For home builds: use Ethernet via USB-OTG adapter. WiFi is unreliable.
- For office builds: WiFi (LEnet, 192.168.0.232) is fine.
- Scripts now read `$env:PI_HOST` (override) → fall back to mDNS hostname
  `digitalcal-pi.local` → so the same scripts work anywhere with one
  env var.
- `sync_to_pi.ps1` now auto-mirrors every subfolder under local `src/`
  on the Pi (Phase 2.2.3 added `src/calendar/` which the previous
  hard-coded `mkdir` missed).
- Don't recommend manual `route add` for VPN-related issues. Either
  disconnect VPN or use the VPN client's "Allow LAN" setting. Manual
  routes leave artefacts that are hard to detect later.

---

## 10. Pending / Known Issues

- **`src/main.cpp`** is an orphan from an earlier mistake. Not compiled (only `main.c` is in `add_executable`). Safe to delete eventually.
- **Subtitle "Phase 2.2.x"** is hardcoded in `theme.cpp` — must update each phase manually.
- **Today = hardcoded 8** in `mini_calendar.cpp` — being fixed in Phase 2.2.4 (real time).
- **DOW for May 1 = hardcoded 4 (Thursday)** — being fixed in Phase 2.2.4 (real time).
- **Phase 2.2.3 self-test prints in `main.c`** — diagnostic output to stderr. Remove or guard once `mini_calendar` consumes thai_calendar (Phase 2.2.5+).
- ~~**No Git repo yet for Pi project**~~ — DONE in 2.2.3 (`git init -b main`, initial commit `89f4439`). ESP32 repo: `thanatvipulakorn-arch/digital_calendar` (master).
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

## 12. Project Status — SHIPPED (v1.0.0, 2026-05-09)

The 2.2.5x batch is the production-ready release. The HDMI display
shows: header (weekday/BE-date/lunar | scaled clock | WiFi+NTP status |
wanphra indicator), full-screen 1920x1080 wallpaper, mini calendar with
Thai-tradition DOW colours and per-day holiday/wanphra markers, weather
stub card, and upcoming holidays list.

What's left (intentionally deferred — not blockers for v1.0):

| Item | Why deferred | Pickup notes |
|------|--------------|--------------|
| Phase 2.2.7-NET (live weather) | Stub renders fine; libcurl + cJSON wiring is a clean follow-on | `apt install libcurl4-openssl-dev libcjson-dev` then add to CMakeLists; replace static "32 / 35 / 65%" in weather.cpp with periodic Open-Meteo fetch (5-min lv_timer in a worker pthread; same code shape as `header_tick`). |
| Phase 2.2.10 (Sidebar tabs) | Single-screen UI is enough for the wall-clock use case | Port `ui_charts.cpp` (~600 lines) and `ui_settings.cpp` (~700 lines) from ESP32 if a multi-screen build becomes useful. |
| Midnight rollover | Snapshots at startup are correct for first 24 h; restart picks up the new day | Wire a day_changed signal in `header_tick` (already detected) to call `mini_calendar_build` / `upcoming_build` again. Same scheme as ESP32. |
| WSL2 desktop simulator | ccache + sync v2 made Pi build cycle 22-60 sec — fast enough | Build LVGL on Linux desktop for sub-second iteration if needed. |

## 13. Next Session Checklist

**Resume order, top to bottom:**

### A. Quick verify the Pi is in the state we left it

1. **SSH reachable.** Default to Ethernet `192.168.1.107`; fall back to mDNS `digitalcal-pi.local`. The `sync_to_pi.ps1` / `build_pi.ps1` scripts already honour `$env:PI_HOST` overrides.
2. **lvglsim binary built.** `ssh thanat@192.168.1.107 "ls -la ~/digital_calendar_pi/build/bin/lvglsim"` — the May 9 session ended with binary built and tmux session `app` running it.
3. **HDMI screen.** Should show: header (clock/date/lunar/wanphra), Phase-2.2.5h-translucent mini calendar (1368×832 with light blue-white grid lines), Weather stub top-right, Upcoming holidays bottom-right. Subtitle bottom-left: "Phase 2.2.5d - widget bigger + grid + weather stub" (subtitle string didn't get bumped past 2.2.5d — cosmetic).
4. **ccache.** `ssh thanat@192.168.1.107 "ccache -s | head -8"` — should show ~590 cacheable / mostly hits. If cache empty for some reason, the next CMakeLists touch is back to ~70 min.

### B. Lek's visual verification of 2.2.5h (if not done yet)

The session ended right after 2.2.5h was built and `lvglsim` restarted, but Lek didn't confirm the screen. Three checks:

- 🪟 **Card translucency** — wallpaper bleeds through more visibly than 2.2.5g
- 📐 **Grid lines** — light blue-white, ~78% opa, fully continuous (no gaps)
- 🌡️ **Weather "35°C"** — degree symbol renders (no ☐)

If anything is off, iterate (~30-60 sec build cycle thanks to ccache + sync v2). If all good, **`git tag v0.2.5h`** and move on.

### C. Decide next batch

| Path | What | Build cost |
|------|------|-----------|
| **Phase 2.2.7-NET** (Batch B) | Replace weather stub with libcurl + cJSON + Open-Meteo. Needs `apt install libcurl4-openssl-dev` on Pi, then add to CMakeLists. Single batch of ~5-8 min build. | ~5-8 min once |
| **Phase 2.2.10** (Batch C) | Sidebar tabs (Home / Month / Settings) — port `ui_charts.cpp` (~600 lines) and `ui_settings.cpp` (~700 lines) from ESP32. Big refactor. | ~10 min |
| **WSL2 desktop simulator** (infra) | Build LVGL on Linux desktop instead of Pi for UI iteration. Section 13 has flagged this as highest-ROI infra investment for ages. With ccache now fixing build speed on Pi, the urgency is lower — but WSL2 cycle would be 2-5 sec, vs Pi's 30-60 sec. | One-time setup ~1-2 hr |
| **2.2.4-MIDNIGHT** | Wire midnight rollover so the `mini_calendar` and `header` rebuild themselves at 00:00:00. Uses `header_tick`'s existing `day_changed` signal. Small. | ~30-60 sec |

### D. Read these before doing anything risky

- `.claude/skills/pi-lvgl-build-workflow/SKILL.md` — every workflow gotcha from May 9. Notable: don't run `cmake ..` without `-DCMAKE_BUILD_TYPE=Release` (now pinned in CMakeLists, but old habits); don't regress sync_to_pi.ps1 to v1; LVGL 9 needs casts on OR'd enums.
- This file's §9 (Failed Experiments) — has the home-WiFi rabbit hole and the lv_conf strip story.

### E. What was NOT touched today (carry-over from earlier sessions)

- `src/main.cpp` orphan still on disk. Not compiled. Safe to delete with `git rm`.
- Mini calendar **midnight refresh** still missing. Snapshot at build only.
- Upcoming holidays **midnight refresh** still missing. Snapshot at build only.
- Buzzer / GPIO timer feature — explicitly de-scoped (timer card removed in 2.2.5d).

---

## 13. Key Decisions Log

- **2026-05-06** — Pi Zero W chosen as production target (vs Pi 4/5) because use case = static wall calendar.
- **2026-05-06** — `lv_port_linux` template chosen over `lv_port_pc_eclipse` to avoid ARMv6 Helium ASM issue with LVGL 9.
- **2026-05-07** — Resolution set to 1280x720 (HDMI native), layout tokens scaled 1.5x from ESP32's 800x480.
- **2026-05-07** — File transfer via scp + PowerShell scripts (not VS Code SFTP — unreliable for new files).
- **2026-05-08** — Thai font kept at 24px only (vs ESP32's 14/24/stacked) to limit assets folder size.
- **2026-05-08** — Aggressive lv_conf optimization deferred until Desktop simulator is available.
- **2026-05-08** — Desktop simulator (WSL2 + LVGL PC port) is highest-ROI next infrastructure investment.
- **2026-05-09** — Phase 2.2.3 ported `thai_calendar.{h,cpp}` verbatim (math-only, no Arduino deps). Self-test on Pi matched myhora.com for 6 reference dates → math correct.
- **2026-05-09** — Home builds use Pi Ethernet via USB-OTG adapter. Home Wi-Fi has wireless-layer block that's not worth fighting; office (LEnet) WiFi is fine.
- **2026-05-09** — `sync_to_pi.ps1` and `build_pi.ps1` now read `$env:PI_HOST` (override) → fall back to `digitalcal-pi.local` (mDNS).
- **2026-05-09** — Phase 2.2.5 background uses `LV_IMAGE_ALIGN_STRETCH` (LVGL 9 idiom) — earlier `lv_image_set_scale_x/y` left widget bbox at 800x480 and the stretched pixels were clipped.
- **2026-05-09** — Switched build process to **batch by phase**: realistic full-rebuild on Pi Zero W is ~1 hour (not 5-10 min as initially estimated). Plan future work as Batch A (UI: 2.2.5/6/8/9), Batch B (network: 2.2.7), Batch C (refactor: 2.2.10) so each only triggers one full rebuild instead of one per sub-phase.
- **2026-05-09** — Phase 2.2.9 timer card: REMOVED permanently. The card was a visual placeholder; the actual feature (countdown + buzzer) needs a GPIO/audio HAT decision that hasn't been made.
- **2026-05-09 (2.2.5d)** — Native-resolution wallpaper: regenerate `bg_calendar.c` directly at 1920x1080 RGB565 from `Calendarbackground_2.jpg`. Avoids the 2.4x upscale blur of the old bg_tulip.
- **2026-05-09 (2.2.5d)** — Sweep `thai_sarabun_24` → `thai_sarabun_stacked_24` for every UI label. The stacked variant raises tone marks +6 px so words like ที่ / นี้ / ขึ้น render correctly. Same character range, 4 px taller line_height.
- **2026-05-09 (2.2.5d)** — **ccache + CMAKE_BUILD_TYPE pin** adopted as a permanent fixture. Cuts CMakeLists-touching rebuilds from 70 min → 3-5 min. Trade-off: first build with ccache empty still takes ~70 min, but only once.
- **2026-05-09 (2.2.5d)** — `sync_to_pi.ps1` v2 (checksum-aware) committed. Skip-if-MD5-matches; never regress to v1. Verified at end-to-end: 39/40 files skipped on a no-op sync, full rebuild dropped from 60 min to 18 sec.
- **2026-05-09 (2.2.5d)** — Captured the day's lessons in `.claude/skills/pi-lvgl-build-workflow/SKILL.md` so the next AI session inherits all the workflow gotchas (ccache mandate, sync v2, network failure tree, LVGL 9 idioms, Thai stacked-font rule).
