# UTF-8 PowerShell Script
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$Host.UI.RawUI.WindowTitle = "上帝野瘋狂 Steam 英文版還原工具"

Write-Host "========================================================" -ForegroundColor Cyan
Write-Host "  《上帝野瘋狂》Steam 英文版還原工具" -ForegroundColor Yellow
Write-Host "========================================================" -ForegroundColor Cyan
Write-Host ""

$SteamDir = "C:\Program Files (x86)\Steam\steamapps\common\Populous 3"
if (-not (Test-Path $SteamDir)) {
    Write-Host "[錯誤] 找不到 Steam 版目錄: $SteamDir" -ForegroundColor Red
    exit 1
}

Write-Host "正在切換登錄檔為英文 (Language=0)..." -ForegroundColor Green
$RegPathHKCU = "HKCU:\Software\Bullfrog Productions Ltd\Populous: The Beginning"
if (Test-Path $RegPathHKCU) {
    Set-ItemProperty -Path $RegPathHKCU -Name "Language" -Value 0 -Type DWord
}

$BakLang = Join-Path $SteamDir "language\lang09.dat.bak"
$DstLang = Join-Path $SteamDir "language\lang09.dat"
if (Test-Path $BakLang) {
    Copy-Item $BakLang $DstLang -Force
}

Write-Host ""
Write-Host "已成功切換回英文版！" -ForegroundColor Green
Write-Host ""
