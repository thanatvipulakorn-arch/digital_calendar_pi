# Calendar wall-display systemd units

Schedule the Samsung Odyssey G3 monitor to power off at night (19:00).
The user wakes it manually in the morning with the monitor's power
button — see "Why no wake-on-DDC?" below.

The `lvglsim` calendar process keeps running the whole time. Only
the monitor power state changes; the Pi continues to render the UI
to its framebuffer.

## Files

| File | Purpose |
|------|---------|
| `calendar-screen-off.service` | One-shot — `ddcutil setvcp D6 04` (DPM Off) |
| `calendar-screen-off.timer`   | Schedule the off-service for 19:00 daily |

`Persistent=true` is set on the timer, so if the Pi was offline when
19:00 fired, the service runs once on next boot to catch up.

## Prerequisites

```bash
# DDC/CI tooling on the Pi
sudo apt install -y ddcutil
```

`i2c-dev` kernel module is loaded automatically by the service via
`ExecStartPre=/sbin/modprobe i2c-dev`, so no `/etc/modules-load.d/`
entry is required.

## Install on the Pi

From the project root on the Windows host (sync the systemd folder
manually since `sync_to_pi.ps1` only mirrors `src/`):

```powershell
$pi = "192.168.1.106"   # or "192.168.1.107" on Ethernet
ssh thanat@$pi "mkdir -p ~/digital_calendar_pi/systemd"
scp .\systemd\*.service .\systemd\*.timer .\systemd\README.md thanat@${pi}:~/digital_calendar_pi/systemd/
```

Then on the Pi:

```bash
ssh thanat@192.168.1.106
sudo cp ~/digital_calendar_pi/systemd/calendar-screen-off.service /etc/systemd/system/
sudo cp ~/digital_calendar_pi/systemd/calendar-screen-off.timer   /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now calendar-screen-off.timer
systemctl list-timers calendar-screen-off.timer
```

The last command should show one upcoming trigger (the next 19:00).

## Verify

```bash
# Manually trigger off to test (monitor should power off in ~1-2 sec).
sudo systemctl start calendar-screen-off.service
# Wake the monitor with the power button.

# View the timer history / next firing time.
systemctl list-timers calendar-screen-off.timer

# Tail the service logs.
journalctl -u calendar-screen-off.service --since today
```

## Disable / remove

```bash
sudo systemctl disable --now calendar-screen-off.timer
sudo rm /etc/systemd/system/calendar-screen-off.{service,timer}
sudo systemctl daemon-reload
```

## Change the schedule

Edit the `OnCalendar=` line in the timer file, then:

```bash
sudo cp ~/digital_calendar_pi/systemd/calendar-screen-off.timer /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl restart calendar-screen-off.timer
```

`OnCalendar=` syntax: `*-*-* HH:MM:SS` for daily, or e.g.
`Mon..Fri 19:00:00` to skip weekends. See `man systemd.time`.

## Why no wake-on-DDC?

A long story discovered while building this feature. Captured here so
the next session doesn't repeat it.

The Pi runs Raspberry Pi OS Trixie with the `vc4-kms-v3d` driver,
so the kernel's KMS layer holds the DRM master. That means none of
the standard "screen off" mechanisms work:

| Method | Why it failed |
|--------|---------------|
| `vcgencmd display_power 0` | Legacy firmware mailbox call — silently no-op on KMS |
| `echo Off > /sys/class/drm/card0-HDMI-A-1/dpms` | Kernel rejects the write while KMS owns the connector (`Permission denied`) |
| HDMI-CEC (`cec-client standby 0`) | This is a computer monitor (Samsung LS27AG32x = Odyssey G3 144Hz gaming), not a TV — does not implement CEC |
| DDC/CI `setvcp D6 04` (DPM Off) | Powers off ✅ but closes the monitor's DDC controller too — wake commands return "Display not found" |
| DDC/CI `setvcp D6 02` (undocumented soft) | Same as 04 — DDC bus closes |
| DDC/CI `--maxtries=20,10,10 setvcp D6 01` | Retry flood doesn't wake the I2C controller |

The only realistic auto-wake paths are (a) cycle the HDMI signal via a
custom libdrm program (LVGL would need to release DRM master first —
significant refactor of the FBDEV backend) or (b) a smart plug that
power-cycles the monitor's AC.

Lek chose to keep this simple: auto off at night, manual wake in the
morning with the power button. That's what these unit files do.

If you want the auto-wake one day:
- Smart plug (TP-Link Kasa, Sonoff) wired to the monitor's mains, with
  systemd timers driving the plug's HTTP/MQTT API. Cleanest option.
- Switch the LVGL backend from FBDEV → DRM (the `lib/display_backends/drm.c`
  file is in the lv_port_linux template) so lvglsim becomes DRM master and
  can `drmModeSetCrtc` the connector off and back on. This makes the
  monitor see "no signal" → "new signal" and wake on its own.
