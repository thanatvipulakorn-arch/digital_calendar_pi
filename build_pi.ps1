# ================================================================
# build_pi.ps1 - Build project on Pi via tmux
#
# Usage:
#   .\build_pi.ps1              # Normal incremental build
#   .\build_pi.ps1 -Reconfigure # Re-run cmake (when CMakeLists.txt changed)
#
# What it does:
#   1. Kills any existing 'build' tmux session
#   2. Starts new tmux session named 'build' on Pi
#   3. Runs cmake (if -Reconfigure) + make -j1
#   4. Attaches to tmux so you see live output
#
# Detach with Ctrl+B then D (build keeps running on Pi).
# ================================================================

param(
    [switch]$Reconfigure
)

$pi = "thanat@192.168.0.232"
$proj_remote = "~/digital_calendar_pi"

# ---- Kill old session ----
Write-Host "Killing old tmux build session..." -ForegroundColor Yellow
ssh $pi "tmux kill-session -t build 2>/dev/null; true" 2>&1 | Out-Null

# ---- Build command ----
if ($Reconfigure) {
    Write-Host "Mode: Reconfigure + Build" -ForegroundColor Cyan
    $cmd = "cd $proj_remote/build && cmake .. && make -j1 2>&1; echo === BUILD_DONE ===; bash"
} else {
    Write-Host "Mode: Incremental Build" -ForegroundColor Cyan
    $cmd = "cd $proj_remote/build && make -j1 2>&1; echo === BUILD_DONE ===; bash"
}

Write-Host ""
Write-Host "Starting build in tmux..." -ForegroundColor Yellow
Write-Host "Detach: Ctrl+B then D" -ForegroundColor Gray
Write-Host ""

# ---- Run in tmux ----
ssh $pi -t "tmux new -s build '$cmd'"
