# ================================================================
# sync_to_pi.ps1 v2 - Sync project files Windows -> Pi
#
# Usage:
#   cd "D:\MY WORK\RASPBERRY PI PROJECT"
#   .\sync_to_pi.ps1                # checksum-aware: skip identical
#   .\sync_to_pi.ps1 -Force         # always copy (legacy v1 behaviour)
#
# What it does:
#   1. Ensures Pi has every src/ subfolder
#   2. Computes MD5 of every candidate file (local + remote in one ssh)
#   3. scp ONLY files whose checksum differs (or are missing on Pi)
#   4. Reports each file: COPIED / skipped / FAIL
#
# Why v2 matters:
#   v1 scp'd every file unconditionally. That bumped the mtime of files
#   like CMakeLists.txt and lv_conf.defaults even when their content was
#   identical. CMake's "config dependency newer than build" check then
#   regenerated lv_conf.h, whose mtime invalidated every .o that included
#   it -> full LVGL recompile (~50 min on Pi Zero W). v2 leaves identical
#   files alone so make's dependency cache stays valid -> incremental
#   builds are actually incremental.
# ================================================================

param(
    [switch]$Force
)

$ErrorActionPreference = "Stop"

# Pi host resolution: $env:PI_HOST overrides the mDNS default
$pi_host = if ($env:PI_HOST) { $env:PI_HOST } else { "digitalcal-pi.local" }
$pi = "thanat@$pi_host"
$proj_local = "D:\MY WORK\RASPBERRY PI PROJECT"
$proj_remote = "~/digital_calendar_pi"

Write-Host ""
Write-Host "==============================================" -ForegroundColor Cyan
Write-Host "  Sync Project to Pi: $pi" -ForegroundColor Cyan
if ($Force) {
    Write-Host "  Mode: FORCE (skip checksum, always copy)" -ForegroundColor Yellow
} else {
    Write-Host "  Mode: checksum-aware (skip identical)" -ForegroundColor Green
}
Write-Host "==============================================" -ForegroundColor Cyan
Write-Host ""

# ---- Step 1: Ensure remote folders exist ------------------------------
Write-Host "[1/4] Ensuring remote folders exist..." -ForegroundColor Yellow
$src_dirs = Get-ChildItem -Path "$proj_local\src" -Recurse -Directory -ErrorAction SilentlyContinue |
    ForEach-Object { ($_.FullName.Substring($proj_local.Length + 1) -replace '\\','/') }
$mkdir_args = @("$proj_remote/src") + ($src_dirs | ForEach-Object { "$proj_remote/$_" })
ssh $pi "mkdir -p $($mkdir_args -join ' ')"
Write-Host "      OK ($($src_dirs.Count + 1) folders)" -ForegroundColor Green
Write-Host ""

# ---- Step 2: Scan local files -----------------------------------------
Write-Host "[2/4] Scanning local files..." -ForegroundColor Yellow

$exclude_patterns = @("*.bak", "*.tmp", "*.pyc")
$files_to_sync = @()

$top_files = @(
    "CMakeLists.txt",
    "lv_conf.defaults",
    "sync_to_pi.ps1"
)
foreach ($f in $top_files) {
    $full = Join-Path $proj_local $f
    if (Test-Path $full) {
        $files_to_sync += @{Local=$full; Remote="$proj_remote/$f"; Name=$f}
    }
}

$src_files = Get-ChildItem -Path "$proj_local\src" -Recurse -File -ErrorAction SilentlyContinue
foreach ($f in $src_files) {
    $skip = $false
    foreach ($pat in $exclude_patterns) {
        if ($f.Name -like $pat) { $skip = $true; break }
    }
    if ($skip) { continue }

    $rel = $f.FullName.Substring($proj_local.Length + 1).Replace("\", "/")
    $files_to_sync += @{Local=$f.FullName; Remote="$proj_remote/$rel"; Name=$f.Name}
}

Write-Host "      Found $($files_to_sync.Count) candidate files" -ForegroundColor Green
Write-Host ""

# ---- Step 3: Compute MD5 (local in-process, remote in 1 ssh round-trip)
$to_copy = @()
$skipped = 0

if ($Force) {
    Write-Host "[3/4] Force mode -- skipping checksum compare" -ForegroundColor Yellow
    $to_copy = $files_to_sync
} else {
    Write-Host "[3/4] Computing checksums (local + remote)..." -ForegroundColor Yellow

    # Resolve $proj_remote on the Pi so md5sum gets absolute paths.
    # Single-quoted string here keeps PowerShell from parsing `&&` / `>`.
    $proj_remote_abs = (ssh $pi 'cd ~/digital_calendar_pi && pwd').Trim()
    if (-not $proj_remote_abs) {
        Write-Host "      [warn] could not resolve $proj_remote -- falling back" -ForegroundColor Yellow
        $proj_remote_abs = "/home/thanat/digital_calendar_pi"
    }

    # Build remote-path list (absolute) for md5sum
    $remote_paths = $files_to_sync | ForEach-Object {
        $_.Remote -replace '^~/digital_calendar_pi', $proj_remote_abs
    }

    # Single ssh: md5sum every candidate. Missing files go to stderr -- we
    # don't care, the regex below filters anything that isn't <hash> <path>.
    # PS5.1 chokes on `2>/dev/null` inside a "+"-built double-quoted string,
    # so build the inner argument from a variable and suppress ssh stderr
    # at the PowerShell level instead.
    $quoted_paths = ($remote_paths | ForEach-Object { "'" + $_ + "'" }) -join " "
    $cmd = "md5sum -- $quoted_paths"
    $remote_output = ssh $pi $cmd 2>$null

    # Parse "<hash>  <path>" into a hashtable
    $remote_hashes = @{}
    foreach ($line in $remote_output) {
        if ($line -match '^([0-9a-f]{32})\s+(.+)$') {
            $remote_hashes[$Matches[2]] = $Matches[1]
        }
    }
    Write-Host "      Got $($remote_hashes.Count) remote hashes" -ForegroundColor Gray

    foreach ($f in $files_to_sync) {
        $remote_abs = $f.Remote -replace '^~/digital_calendar_pi', $proj_remote_abs
        $local_md5  = (Get-FileHash -Path $f.Local -Algorithm MD5).Hash.ToLower()
        $remote_md5 = $remote_hashes[$remote_abs]

        if ($remote_md5 -eq $local_md5) {
            $skipped++
        } else {
            $to_copy += $f
        }
    }
    Write-Host "      Identical: $skipped   Need copy: $($to_copy.Count)" -ForegroundColor Green
}
Write-Host ""

# ---- Step 4: scp the changed files ------------------------------------
$success = 0
$failed  = 0

if ($to_copy.Count -eq 0) {
    Write-Host "[4/4] Nothing to copy -- Pi already in sync" -ForegroundColor Green
} else {
    Write-Host "[4/4] Copying $($to_copy.Count) file(s)..." -ForegroundColor Yellow
    foreach ($item in $to_copy) {
        Write-Host "      -> $($item.Name) " -NoNewline
        try {
            scp -q $item.Local "${pi}:$($item.Remote)" 2>&1 | Out-Null
            if ($LASTEXITCODE -eq 0) {
                Write-Host "OK" -ForegroundColor Green
                $success++
            } else {
                Write-Host "FAIL" -ForegroundColor Red
                $failed++
            }
        } catch {
            Write-Host "ERROR: $_" -ForegroundColor Red
            $failed++
        }
    }
}

Write-Host ""
Write-Host "==============================================" -ForegroundColor Cyan
Write-Host "  $success copied, $skipped skipped, $failed failed" -ForegroundColor Cyan
Write-Host "==============================================" -ForegroundColor Cyan
Write-Host ""

if ($failed -ne 0) {
    Write-Host "Some files failed. Check network or paths." -ForegroundColor Red
} elseif ($success -eq 0) {
    Write-Host "Tip: nothing changed since last sync -- your incremental build" -ForegroundColor Gray
    Write-Host "     should be a no-op (just a make timestamp check)." -ForegroundColor Gray
} else {
    Write-Host "Next:  ssh $pi -t `"cd $proj_remote/build && make -j1`"" -ForegroundColor Gray
}
