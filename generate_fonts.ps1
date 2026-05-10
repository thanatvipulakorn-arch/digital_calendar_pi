# =====================================================================
# generate_fonts.ps1 - generate LVGL bitmap fonts from Sarabun-Regular.ttf
#
# Usage:
#   .\generate_fonts.ps1                    # generate the default set
#   .\generate_fonts.ps1 -Sizes 24,48,72    # generate specific sizes
#
# Requires:
#   - Node.js + npm
#   - lv_font_conv (npm install -g lv_font_conv)
#   - fonts/Sarabun-Regular.ttf (download from Google Fonts if missing)
#
# Outputs to: src/assets/thai_sarabun_<size>.c
#
# Ranges match the existing thai_sarabun_24.c (kept for consistency):
#   32-127      ASCII printable
#   3584-3711   Thai block (U+0E00 .. U+0E7F)
# =====================================================================

param(
    [int[]]$Sizes = @(72)
)

$ErrorActionPreference = "Stop"

$projRoot = $PSScriptRoot
$ttf      = Join-Path $projRoot "fonts\Sarabun-Regular.ttf"
$outDir   = Join-Path $projRoot "src\assets"

if (-not (Test-Path $ttf)) {
    throw "Sarabun-Regular.ttf not found at $ttf - download from Google Fonts first"
}
if (-not (Test-Path $outDir)) {
    throw "Output directory $outDir does not exist"
}

foreach ($size in $Sizes) {
    $outName = "thai_sarabun_$size.c"
    $outPath = Join-Path $outDir $outName
    Write-Host ""
    Write-Host "Generating $outName (size=$size)..." -ForegroundColor Cyan

    # Note: --stride / --align were removed in lv_font_conv >=1.5.
    # The current bitmap layout is fixed; output is byte-equivalent.
    & lv_font_conv `
        --bpp 4 `
        --size $size `
        --no-compress `
        --font $ttf `
        --range "32-127,3584-3711" `
        --format lvgl `
        -o $outPath

    if ($LASTEXITCODE -ne 0) {
        throw "lv_font_conv failed for size $size"
    }
    $info = Get-Item $outPath
    $sizeStr = "{0:N0}" -f $info.Length
    Write-Host "  OK  $($info.Name)  ($sizeStr bytes)" -ForegroundColor Green
}

Write-Host ""
Write-Host "Done. Add the new file(s) to:" -ForegroundColor Yellow
Write-Host "  - src/assets/thai_fonts.h    (LV_FONT_DECLARE)"
Write-Host "  - CMakeLists.txt             (add_executable / target_sources)"
