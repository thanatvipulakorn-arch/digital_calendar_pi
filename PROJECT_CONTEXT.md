# Digital Calendar Pi — Project Context

> **Single source of truth** for the Pi Zero W Digital Calendar project.
> Last updated: **May 9, 2026** (Phase 2.2.5 / 2.2.6 / 2.2.8 / 2.2.9 batch — pending verify)

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
| 2.2.5 Tulip background | BATCH-A pending verify | May 9 | bg_tulip.c image (4.49 MB, 800x480 RGB565 ported from ESP32), full-screen via `LV_IMAGE_ALIGN_STRETCH`. mini_calendar/upcoming/timer cards translucent (opa 220). |
| 2.2.6 Header | BATCH-A pending verify | May 9 | `header.{h,cpp}` — weekday/date/lunar/clock/wanphra. 1 Hz lv_timer in main.c → `header_tick()` updates clock per-second, others on day_changed. |
| 2.2.7 Weather card | PENDING (Batch B) | — | OpenWeather API, libcurl |
| 2.2.8 Upcoming holidays | BATCH-A pending verify | May 9 | `upcoming.{h,cpp}` — scans 90 days via `thai_calendar_*`, sorts ascending, shows up to 5 with offset chip. Snapshot at build (no midnight refresh yet). |
| 2.2.9 Timer | BATCH-A pending verify | May 9 | `timer_card.{h,cpp}` — visual placeholder (bell + "Tap to set"). State machine + popup + buzzer deferred (need GPIO/audio HAT). |
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
- **2026-05-09** — Phase 2.2.3 ported `thai_calendar.{h,cpp}` verbatim (math-only, no Arduino deps). Self-test on Pi matched myhora.com for 6 reference dates → math correct.
- **2026-05-09** — Home builds use Pi Ethernet via USB-OTG adapter. Home Wi-Fi has wireless-layer block that's not worth fighting; office (LEnet) WiFi is fine.
- **2026-05-09** — `sync_to_pi.ps1` and `build_pi.ps1` now read `$env:PI_HOST` (override) → fall back to `digitalcal-pi.local` (mDNS).
- **2026-05-09** — Phase 2.2.5 background uses `LV_IMAGE_ALIGN_STRETCH` (LVGL 9 idiom) — earlier `lv_image_set_scale_x/y` left widget bbox at 800x480 and the stretched pixels were clipped.
- **2026-05-09** — Switched build process to **batch by phase**: realistic full-rebuild on Pi Zero W is ~1 hour (not 5-10 min as initially estimated). Plan future work as Batch A (UI: 2.2.5/6/8/9), Batch B (network: 2.2.7), Batch C (refactor: 2.2.10) so each only triggers one full rebuild instead of one per sub-phase.
- **2026-05-09** — Phase 2.2.9 timer card: visual placeholder only (bell + "Tap to set"). State machine + popup + buzzer deferred — needs explicit GPIO/audio HAT decision.
