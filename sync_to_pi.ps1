# ================================================================
# sync_to_pi.ps1 - Sync project files from Windows → Pi
#
# Usage:
#   cd "D:\MY WORK\RASPBERRY PI PROJECT"
#   .\sync_to_pi.ps1
#
# What it does:
#   1. Ensures Pi has required folders
#   2. Copies all source files (src/, CMakeLists.txt, lv_conf.defaults)
#   3. Reports each file copied
#   4. Skips: build/, lvgl/, .git/, *.bak (to save time)
#
# After sync: SSH to Pi and run build manually.
# ================================================================

$ErrorActionPreference = "Stop"
$pi = "thanat@192.168.0.232"
$proj_local = "D:\MY WORK\RASPBERRY PI PROJECT"
$proj_remote = "~/digital_calendar_pi"

Write-Host ""
Write-Host "==============================================" -ForegroundColor Cyan
Write-Host "  Sync Project to Pi: $pi" -ForegroundColor Cyan
Write-Host "==============================================" -ForegroundColor Cyan
Write-Host ""

# ---- Step 1: Ensure folders exist on Pi ----
Write-Host "[1/3] Ensuring remote folders exist..." -ForegroundColor Yellow
ssh $pi "mkdir -p $proj_remote/src/ui $proj_remote/src/assets $proj_remote/src/lib"
Write-Host "      OK" -ForegroundColor Green
Write-Host ""

# ---- Step 2: Build file list (recursive, exclude unwanted) ----
Write-Host "[2/3] Scanning local files..." -ForegroundColor Yellow

$exclude_dirs = @("build", "lvgl", ".git", ".vscode", "_old", "backup")
$exclude_patterns = @("*.bak", "*.tmp", "*.pyc")

$files_to_sync = @()

# Top-level files
$top_files = @(
    "CMakeLists.txt",
    "lv_conf.defaults",
    "sync_to_pi.ps1"
)
foreach ($f in $top_files) {
    $full = Join-Path $proj_local $f
    if (Test-Path $full) {
        $files_to_sync += @{Local=$full; Remote="$proj_remote/$f"}
    }
}

# src/ folder (recursive)
$src_files = Get-ChildItem -Path "$proj_local\src" -Recurse -File -ErrorAction SilentlyContinue
foreach ($f in $src_files) {
    # Skip excluded patterns
    $skip = $false
    foreach ($pat in $exclude_patterns) {
        if ($f.Name -like $pat) { $skip = $true; break }
    }
    if ($skip) { continue }
    
    # Compute remote path (preserve folder structure)
    $rel = $f.FullName.Substring($proj_local.Length + 1).Replace("\", "/")
    $files_to_sync += @{Local=$f.FullName; Remote="$proj_remote/$rel"}
}

Write-Host "      Found $($files_to_sync.Count) files to sync" -ForegroundColor Green
Write-Host ""

# ---- Step 3: SCP each file ----
Write-Host "[3/3] Copying files to Pi..." -ForegroundColor Yellow
$success = 0
$failed = 0

foreach ($item in $files_to_sync) {
    $local = $item.Local
    $remote = $item.Remote
    $name = Split-Path -Leaf $local
    
    Write-Host "      -> $name " -NoNewline
    
    try {
        scp -q $local "${pi}:${remote}" 2>&1 | Out-Null
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

Write-Host ""
Write-Host "==============================================" -ForegroundColor Cyan
Write-Host "  Sync complete: $success ok, $failed failed" -ForegroundColor Cyan
Write-Host "==============================================" -ForegroundColor Cyan
Write-Host ""

if ($failed -eq 0) {
    Write-Host "Next steps:" -ForegroundColor White
    Write-Host "  1. Build:  ssh $pi -t `"tmux new -s build 'cd $proj_remote/build && cmake .. && make -j1 2>&1; echo === BUILD_DONE ===; bash'`"" -ForegroundColor Gray
    Write-Host "  2. Run:    ssh $pi `"cd $proj_remote/build && ./bin/lvglsim`"" -ForegroundColor Gray
    Write-Host "  3. Or:     ssh $pi  (then type: cal)" -ForegroundColor Gray
    Write-Host ""
} else {
    Write-Host "Some files failed. Check network or paths." -ForegroundColor Red
}
