---
name: pi-lvgl-build-workflow
description: Use when working on Raspberry Pi LVGL projects (especially Pi Zero W) — covers sync, build cache, network setup, LVGL 9 idioms, layout patterns, and timing realities. Trigger when the user mentions any of "Pi", "Raspberry Pi", "digital_calendar", "LVGL", "lvglsim", "sync_to_pi", "build_pi", "ccache", "thai_sarabun", or asks about embedded UI build performance or network access to the Pi. Captures hard-won lessons from a 10-hour debugging session that took build cycles from 60 min → 30 sec.
---

# Pi LVGL Build Workflow

Hard-won ground truths captured at the end of Phase 2.2.5d. Skim the **Non-Negotiables** first; the rest is reference.

## Build Performance — Non-Negotiables

### 1. ccache must be installed and wired in CMakeLists

Pi-side install:
```bash
sudo apt install ccache
```

CMakeLists.txt wiring (top-level, after `project()`):
```cmake
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "ccache found: ${CCACHE_PROGRAM}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()
```

Without ccache, every CMakeLists.txt edit (e.g. adding a new source file at a phase boundary) triggers a full LVGL recompile (~70 min on Pi Zero W). With ccache, the same edit finishes in **~30 sec** because object hashes match → cached `.o` is reused.

Safety: ccache caches by content hash of source + flags + compiler binary. Collisions are astronomically unlikely (10^-18). Reset with `ccache -C` if ever suspicious.

### 2. CMAKE_BUILD_TYPE must be pinned in CMakeLists.txt

A bare `cmake ..` (no `-DCMAKE_BUILD_TYPE`) defaults to empty/Debug → all `.o` invalidated → full rebuild. Pin it:
```cmake
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()
```

### 3. sync_to_pi.ps1 must be checksum-aware (v2)

The "always-copy" v1 script bumps `mtime` of unchanged files (CMakeLists.txt, lv_conf.defaults). CMake's "config dependency newer than build" check then regenerates `lv_conf.h`, whose new mtime invalidates every `.o` that includes it → full LVGL recompile.

v2 fetches remote MD5s in one ssh round-trip and only `scp`s files that actually differ:
- `Get-FileHash -Algorithm MD5` for local
- `ssh thanat@... "md5sum -- file1 file2 ..."` for remote (one call)
- Compare → scp only mismatches

Two PS5.1 traps when writing the script:
- `2>/dev/null` inside a `+`-built double-quoted string trips the parser. Build the args in a variable and use `$cmd 2>$null` at PS level.
- ssh stderr from a missing file (md5sum exit 1) breaks `$ErrorActionPreference = "Stop"`. Wrap in temporary `$ErrorActionPreference = "Continue"` or `try/catch`.
- Save script as **UTF-8 with BOM** so PS5.1 doesn't misparse comment chars.

### 4. Pi Zero W realistic timing (1 GHz ARMv6, 512 MB RAM)

| Scenario | Time |
|----------|------|
| Full LVGL rebuild (no cache) | ~70 min |
| Full rebuild with ccache populated | ~1 min |
| Source-only edit + relink (sync v2 + make) | 20-60 sec |
| Adding source in CMakeLists (with ccache) | 3-5 min |
| Editing `lv_conf.defaults` | ~50 min always (changes ALL `.o` deps) |

**Rule of thumb:** if "incremental" feels slower than 5 min, *something is invalidating the cache*. Investigate immediately — don't accept it as "Pi Zero W is just slow".

## Network — Getting SSH to the Pi

### Layered failure tree (in the order to check)

1. **Pi network** — `ip -4 addr` on Pi must show non-loopback IP on `eth0` or `wlan0`.
2. **Pi sshd** — `sudo systemctl status ssh`. Pi OS Trixie ships with ssh disabled by default; enable via `sudo raspi-config nonint do_ssh 0`.
3. **Windows can ping Pi** — proves L3 reachability.
4. **TCP 22 open** — `Test-NetConnection -Port 22`. If ping passes but TCP fails, suspect Wi-Fi AP isolation or Windows network profile.
5. **Windows network profile = Private** (not Public). Public profile blocks LAN inbound:
   ```powershell
   # Admin
   Set-NetConnectionProfile -Name "<SSID>" -NetworkCategory Private
   ```
6. **Wireless layer block at home router** — symptom: "banner exchange: Connection to UNKNOWN port -1: Connection refused" while ping works. Bypass with **USB-OTG → Ethernet adapter** on the Pi. Office WiFi (LEnet) tends to work fine; consumer home routers may not.
7. **VPN artefacts** — ProtonVPN active or recently disconnected can leave routes/firewall rules that block LAN. Don't use manual `route add` (it persists, then breaks the on-link route when deleted). Use the VPN's own "Allow LAN" toggle, or fully Quit the VPN client.

### Connection helpers (sync_to_pi.ps1 / build_pi.ps1)

Use mDNS hostname with env var override:
```powershell
$pi_host = if ($env:PI_HOST) { $env:PI_HOST } else { "digitalcal-pi.local" }
$pi = "thanat@$pi_host"
```
- Default works on any LAN where Avahi is reachable
- Override for VPN'd Windows (`$env:PI_HOST = "192.168.1.107"`)

### Pi Zero W radio limitations

- 2.4 GHz only (BCM43438 chip). Cannot see 5 GHz networks.
- Phone hotspot: iPhone needs *Personal Hotspot → Maximize Compatibility*; Android needs *Hotspot → AP Band → 2.4 GHz only*.

## LVGL 9 Idioms

### Image stretch to fill a widget

```c
lv_obj_set_size(img, w, h);
lv_image_set_inner_align(img, LV_IMAGE_ALIGN_STRETCH);
```

NOT `lv_image_set_scale_x/y` alone. Scale grows the rendered pixels but keeps the widget bbox at the original image size, so the stretched pixels get clipped to the original area.

### Fonts larger than max builtin (Montserrat 48)

```c
lv_obj_set_style_text_font(lbl, &lv_font_montserrat_48, LV_PART_MAIN);
lv_obj_set_style_transform_scale_x(lbl, 460, LV_PART_MAIN);  /* 1.8x */
lv_obj_set_style_transform_scale_y(lbl, 384, LV_PART_MAIN);  /* 1.5x */
```
256 = 1.0x. Asymmetric scale is fine for glyphs that are mostly straight lines (digits, basic Thai); it'll look stretched on round letters.

### Strongly-typed enums need an explicit cast for OR

```c
/* WRONG (LVGL 9 errors with -fpermissive: invalid conversion from int) */
lv_obj_set_style_border_side(c, LV_BORDER_SIDE_RIGHT | LV_BORDER_SIDE_BOTTOM, 0);

/* RIGHT */
lv_obj_set_style_border_side(c,
    (lv_border_side_t)(LV_BORDER_SIDE_RIGHT | LV_BORDER_SIDE_BOTTOM), 0);
```
Same for `lv_obj_clear_flag(o, FLAG_A | FLAG_B)` — split into two separate calls.

### `lv_obj_remove_style_all()` before custom styling

LVGL's default theme adds bg/border/pad to every `lv_obj_create`. To make a bare grid cell with only a single-side border, strip first:
```c
lv_obj_t *cell = lv_obj_create(parent);
lv_obj_remove_style_all(cell);
/* now add only border on right + bottom */
```

### Center a label without pad math

```c
lv_obj_t *lbl = lv_label_create(parent);
lv_label_set_text(lbl, "X");
lv_obj_center(lbl);  /* no MINI_CELL_H/2 - font/2 calculation */
```

## Display / Resolution

- **Match LVGL window to fbdev.** Check actual: `cat /sys/class/graphics/fb0/virtual_size`. The `lv_port_linux` template defaults to 800x480 (ESP32 legacy) — update `main.c::configure_simulator` defaults to match HDMI native (commonly 1920x1080).
- **EDGE_MARGIN ≥ 24 px to dodge TV overscan.** Many consumer TVs crop 10–30 px at edges. If a card "disappears" near the right/bottom edge, increase the margin.
- **High-res background image generation.** A native 1920x1080 RGB565 C array is ~24 MB source / ~4 MB binary. Don't stretch a small (800x480) image — looks blurry at 2.4x. Use PowerShell + `System.Drawing.Bitmap` + `LockBits` + an inline C# helper for fast (~2 sec) bicubic resample + RGB565 conversion.

## Thai Font Specifics

Use the **stacked variant** (`thai_sarabun_stacked_24`) for any label with vowel + tone marks above. The regular variant renders the tone mark too low → overlaps the vowel. The stacked variant has the tone marks raised +6 px (line_height 38 vs 34); same ASCII + Thai range otherwise.

Cases that NEED the stacked variant:

| Word | Stack |
|------|-------|
| ขึ้น | ึ + ้ |
| ที่ | ี + ่ |
| นี้ | ี + ้ |
| รู้ | ู + ้ |
| ค่ำ | ่ + ำ |

Practical rule: just sweep all `&thai_sarabun_24` references to `&thai_sarabun_stacked_24` for consistency. The stacked variant looks identical for non-stacking text.

LVGL 8 → 9 font compatibility: ESP32-era fonts work on LVGL 9 if they have these `#if` guards already (check the bottom of the .c file):
```c
#if LVGL_VERSION_MAJOR >= 9
    .static_bitmap = 0,
#endif
```

## Workflow Cadence

### Batch related changes into one build

Phase 2.2.5d combined: layout fix + new font + new card stub + ccache install + build-type pin → **one 70-min build** instead of five 70-min builds. Saves 4+ hours.

Group changes by what they touch:
- Pure source edits → quick batch (any time)
- CMakeLists changes → batch with other CMakeLists changes
- `lv_conf.defaults` change → save for last (most expensive)

### Always estimate build time before triggering

Communicate estimate to the user before starting:
- Source-only edit → ~30 sec
- Header changed → 1–2 min
- CMakeLists.txt changed → 3–5 min (ccache) / ~70 min (no ccache)
- `lv_conf.defaults` → ~50 min unconditionally

If reality > 1.5x the estimate, *stop and investigate* what's invalidating cache.

### Long builds via tmux + background watcher

```bash
ssh thanat@... "tmux new-session -d -s build 'cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j1 2>&1 | tee /tmp/build.log; echo BUILD_DONE >> /tmp/build.log'"
```
Watcher (run from local Bash with `run_in_background: true`):
```bash
ssh -o ServerAliveInterval=60 thanat@... \
  "until grep -q BUILD_DONE /tmp/build.log; do sleep 60; done; tail -25 /tmp/build.log"
```

Don't block an interactive ssh session on a 70-min foreground build — it'll drop and lose progress visibility.

### Verify visual changes ASAP

After every build, restart `lvglsim` in tmux and ask the user to look at the HDMI screen before piling on the next change. Multiple untested changes batched together are hard to debug if one breaks layout.

```bash
ssh thanat@... "tmux kill-session -t app 2>/dev/null; pkill -9 lvglsim; sleep 1; \
    tmux new-session -d -s app 'cd build && ./bin/lvglsim 2>&1 | tee /tmp/lvg.log'"
```

## Common Traps Cheatsheet

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| Build takes 70 min for a 1-line change | ccache missing, OR CMakeLists.txt was edited, OR sync v1 | Install ccache; pin BUILD_TYPE; use sync v2 |
| `cmake ..` triggers full rebuild | Missing `-DCMAKE_BUILD_TYPE=Release` | Pin in CMakeLists |
| sync touches every mtime | Always-copy scp loop | sync v2 (MD5 compare) |
| `journalctl -f + tcpdump` on Pi Zero W | RAM swap thrash, console hang | Run one at a time, or via ssh |
| ProtonVPN disconnect leaves Pi unreachable | Manual `route add` persisted | `route delete` + restart Wi-Fi adapter |
| TCP 22 refused but ping OK | Wireless-layer block / Windows profile = Public | USB Ethernet bypass + set profile Private |
| Pi can't see WiFi 5G network | Pi Zero W is 2.4 GHz only | Phone hotspot "compatibility mode" |
| Tone marks invisible / overlapping | Using regular thai_sarabun font | Switch to thai_sarabun_stacked variant |
| LVGL 9 compile error "invalid conversion from int" | OR'd flags passed where typed enum expected | Cast: `(lv_border_side_t)(A | B)` |
| Image renders smaller than expected | `lv_image_set_scale_*` without `lv_obj_set_size` | Use `LV_IMAGE_ALIGN_STRETCH` instead |

## See Also

- `PROJECT_CONTEXT.md` — project state + decision log (Section 9 has more failed-experiment notes)
- `LEK_WORKING_STYLE.md` — Lek's collaboration preferences (project copy + user-scoped at `D:\MY WORK\LEK_WORKING_STYLE.md`)
- ESP32 reference source — `D:\MY WORK\MY EMBEDED PROJECT\ESP32\digital_calendar\` (port from this when adding features)
