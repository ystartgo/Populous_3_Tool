[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$Host.UI.RawUI.WindowTitle = "上帝野瘋狂 Steam 繁體中文化工具"

Write-Host "========================================================" -ForegroundColor Cyan
Write-Host "  《上帝野瘋狂》Steam / Multiverse 一鍵繁體中文化套用工具" -ForegroundColor Yellow
Write-Host "========================================================" -ForegroundColor Cyan
Write-Host ""

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$SteamDir = "C:\Program Files (x86)\Steam\steamapps\common\Populous 3"

if (-not (Test-Path $SteamDir)) {
    Write-Host "[錯誤] 找不到 Steam 版目錄: $SteamDir" -ForegroundColor Red
    exit 1
}

Write-Host "[1/4] 正在注入繁體中文語系檔 (lang09 與 lang00)..." -ForegroundColor Green
$SrcLang = Join-Path $ScriptDir "language\lang09.dat"
$DstLang09 = Join-Path $SteamDir "language\lang09.dat"
$DstLang00 = Join-Path $SteamDir "language\lang00.dat"

Copy-Item $SrcLang $DstLang09 -Force
Copy-Item $SrcLang $DstLang00 -Force

Write-Host "[2/4] 正在複製 Big5 中文字型與介面資源 (B5fnt16/24)..." -ForegroundColor Green
Copy-Item (Join-Path $ScriptDir "data\fenew\*.*") (Join-Path $SteamDir "data\fenew\") -Force
Copy-Item (Join-Path $ScriptDir "data\loadlog3.dat") (Join-Path $SteamDir "data\loadlog3.dat") -Force
Copy-Item (Join-Path $ScriptDir "fmv\p3introc.tgq") (Join-Path $SteamDir "fmv\p3introc.tgq") -Force

Write-Host "[3/4] 正在注入 Multiverse 戰役數據庫 (GameData)..." -ForegroundColor Green
$GData = Join-Path $SteamDir "Multiverse\GameData"
if (Test-Path $GData) {
    Get-ChildItem -Path $GData -Directory | ForEach-Object {
        $slotPath = $_.FullName
        $slotLang = Join-Path $slotPath "language"
        $slotFenew = Join-Path $slotPath "data\fenew"
        if (-not (Test-Path $slotLang)) { New-Item -Path $slotLang -ItemType Directory -Force | Out-Null }
        if (-not (Test-Path $slotFenew)) { New-Item -Path $slotFenew -ItemType Directory -Force | Out-Null }
        Copy-Item $SrcLang (Join-Path $slotLang "lang09.dat") -Force
        Copy-Item $SrcLang (Join-Path $slotLang "lang00.dat") -Force
        Copy-Item (Join-Path $ScriptDir "data\fenew\*.*") $slotFenew -Force
    }
}

Write-Host "[4/4] 正在修補執行檔語系判定 (D3DPopTB.exe, popTB.exe, popTBM.exe)..." -ForegroundColor Green

$csharp = @"
using System;
using System.IO;

public class FastPatcher {
    public static void Patch(string filePath) {
        if (!File.Exists(filePath)) return;
        byte[] b = File.ReadAllBytes(filePath);
        bool modified = false;

        // Pattern 1: Language table [0, 11, 11, 1, 11, 2] -> change 2 to 0
        byte[] p1 = new byte[] { 0, 11, 11, 1, 11, 2 };
        for (int i = 0; i <= b.Length - 6; i++) {
            if (b[i] == 0 && b[i+1] == 11 && b[i+2] == 11 && b[i+3] == 1 && b[i+4] == 11 && b[i+5] == 2) {
                b[i+5] = 0;
                modified = true;
            }
        }

        // Pattern 2: Fallback mov eax, [0x672bb2] (a1 b2 2b 67 00 50) -> mov eax, 9 (b8 09 00 00 00 50)
        byte[] p2 = new byte[] { 0xa1, 0xb2, 0x2b, 0x67, 0x00, 0x50 };
        for (int i = 0; i <= b.Length - 6; i++) {
            if (b[i] == 0xa1 && b[i+1] == 0xb2 && b[i+2] == 0x2b && b[i+3] == 0x67 && b[i+4] == 0x00 && b[i+5] == 0x50) {
                b[i] = 0xb8; b[i+1] = 0x09; b[i+2] = 0x00; b[i+3] = 0x00; b[i+4] = 0x00;
                modified = true;
            }
        }

        // Pattern 3: No-CD check at 0xaaf0 (ret = 0xc3)
        if (b.Length > 0xab00 && b[0xaaf0] != 0xc3) {
            b[0xaaf0] = 0xc3;
            modified = true;
        }

        if (modified) {
            File.WriteAllBytes(filePath, b);
        }
    }
}
"@

Add-Type -TypeDefinition $csharp

$exes = @("D3DPopTB.exe", "D3DPopTBUW.exe", "popTB.exe", "popTBUW.exe", "popTBM.exe")
foreach ($exeName in $exes) {
    $fp = Join-Path $SteamDir $exeName
    [FastPatcher]::Patch($fp)
}

# 註冊表語系修復 (HKCU & HKLM)
try {
    New-Item -Path "HKCU:\SOFTWARE\Bullfrog Productions Ltd\Populous: The Beginning" -Force | Out-Null
    Set-ItemProperty -Path "HKCU:\SOFTWARE\Bullfrog Productions Ltd\Populous: The Beginning" -Name "Language" -Value 1028 -Type DWord -Force
} catch {}

try {
    if ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent().IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        New-Item -Path "HKLM:\SOFTWARE\WOW6432Node\Bullfrog Productions Ltd\Populous: The Beginning" -Force -ErrorAction SilentlyContinue | Out-Null
        Set-ItemProperty -Path "HKLM:\SOFTWARE\WOW6432Node\Bullfrog Productions Ltd\Populous: The Beginning" -Name "Language" -Value 1028 -Type DWord -Force -ErrorAction SilentlyContinue
    }
} catch {}

Write-Host ""
Write-Host "========================================================" -ForegroundColor Cyan
Write-Host "  恭喜！繁體中文化套用完成！" -ForegroundColor Green
Write-Host "  Steam 原生啟動 與 Multiverse 啟動器皆已完全中文化！" -ForegroundColor Yellow
Write-Host "========================================================" -ForegroundColor Cyan
Write-Host ""