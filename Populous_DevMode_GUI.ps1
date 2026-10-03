[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

# 遊戲路徑與設定檔
$GameDir = "C:\Program Files (x86)\Steam\steamapps\common\Populous 3"
if (-not (Test-Path $GameDir)) {
    $GameDir = $PSScriptRoot
}
$LauncherIni = Join-Path $GameDir "launcher.ini"

# 讀取當前 develop_mode
$isDevMode = $false
if (Test-Path $LauncherIni) {
    $content = Get-Content $LauncherIni -Encoding UTF8
    $match = $content | Select-String -Pattern "^develop_mode\s*=\s*(\d+)" | Select-Object -First 1
    if ($match -and $match.Matches.Groups[1].Value -eq "1") {
        $isDevMode = $true
    }
}

# 建立雙語 GUI 視窗（加寬至 730，高度 630）
$form = New-Object System.Windows.Forms.Form
$form.Text = "《上帝也瘋狂 3》密技與開發者模式開關 / Populous 3 Cheat & Develop Mode Switcher"
$form.Size = New-Object System.Drawing.Size(730, 630)
$form.StartPosition = "CenterScreen"
$form.FormBorderStyle = [System.Windows.Forms.FormBorderStyle]::FixedDialog
$form.MaximizeBox = $false
$form.Font = New-Object System.Drawing.Font("Microsoft JhengHei", 9.5)

# 1. 頂部狀態卡片 GroupBox
$grpStatus = New-Object System.Windows.Forms.GroupBox
$grpStatus.Text = "目前密技狀態 / Current Cheat Mode Status"
$grpStatus.Location = New-Object System.Drawing.Point(25, 12)
$grpStatus.Size = New-Object System.Drawing.Size(665, 80)

$lblStatus = New-Object System.Windows.Forms.Label
$lblStatus.Location = New-Object System.Drawing.Point(20, 24)
$lblStatus.Size = New-Object System.Drawing.Size(625, 45)
$lblStatus.Font = New-Object System.Drawing.Font("Microsoft JhengHei", 11.5, [System.Drawing.FontStyle]::Bold)

function Update-StatusLabel {
    if ($rbOn.Checked) {
        $lblStatus.Text = "● 目前狀態：【已開啟】密技模式 (develop_mode=1)`n   Current Status: [ENABLED] Cheats Active"
        $lblStatus.ForeColor = [System.Drawing.Color]::FromArgb(0, 130, 0)
    } else {
        $lblStatus.Text = "○ 目前狀態：【已關閉】普通原版模式 (develop_mode=0)`n   Current Status: [DISABLED] Cheats Inactive"
        $lblStatus.ForeColor = [System.Drawing.Color]::FromArgb(180, 40, 40)
    }
}

$grpStatus.Controls.Add($lblStatus)
$form.Controls.Add($grpStatus)

# 2. 開關切換 GroupBox
$grpSwitch = New-Object System.Windows.Forms.GroupBox
$grpSwitch.Text = "模式設定 / Mode Selection (develop_mode)"
$grpSwitch.Location = New-Object System.Drawing.Point(25, 102)
$grpSwitch.Size = New-Object System.Drawing.Size(665, 140)

$rbOn = New-Object System.Windows.Forms.RadioButton
$rbOn.Text = "【開啟】密技與開發者模式 / ENABLE Cheats & Develop Mode (develop_mode=1)`n  解鎖 byrne 密技、Tab+F1~F5 快速鍵、Alt 游標刷兵刷建築`n  Unlocks 'byrne' console cheat, Tab+F1~F5 shortcuts, and Alt+Q/A/Z/X thing spawner."
$rbOn.Location = New-Object System.Drawing.Point(20, 24)
$rbOn.Size = New-Object System.Drawing.Size(630, 52)
$rbOn.Checked = $isDevMode
$rbOn.Font = New-Object System.Drawing.Font("Microsoft JhengHei", 9.5, [System.Drawing.FontStyle]::Bold)
$rbOn.Add_CheckedChanged({ Update-StatusLabel })

$rbOff = New-Object System.Windows.Forms.RadioButton
$rbOff.Text = "【關閉】普通原版純淨模式 / DISABLE Cheats (Vanilla Mode) (develop_mode=0)`n  停用所有作弊與除錯功能，享受 100% 原版純淨難度挑戰`n  Disables all cheats and debug features for standard gameplay challenge."
$rbOff.Location = New-Object System.Drawing.Point(20, 80)
$rbOff.Size = New-Object System.Drawing.Size(630, 50)
$rbOff.Checked = (-not $isDevMode)
$rbOff.Font = New-Object System.Drawing.Font("Microsoft JhengHei", 9.5)
$rbOff.Add_CheckedChanged({ Update-StatusLabel })

$grpSwitch.Controls.Add($rbOn)
$grpSwitch.Controls.Add($rbOff)
$form.Controls.Add($grpSwitch)

# 3. 密技使用說明 GroupBox
$grpHelp = New-Object System.Windows.Forms.GroupBox
$grpHelp.Text = "可用密技快捷鍵指南 / In-Game Cheats & Shortcuts Reference"
$grpHelp.Location = New-Object System.Drawing.Point(25, 252)
$grpHelp.Size = New-Object System.Drawing.Size(665, 235)

$lblHelp = New-Object System.Windows.Forms.Label
$lblHelp.Location = New-Object System.Drawing.Point(20, 22)
$lblHelp.Size = New-Object System.Drawing.Size(625, 205)
$lblHelp.Font = New-Object System.Drawing.Font("Microsoft JhengHei", 9.0)
$lblHelp.Text = "1.【Tab + F11】跳出對話框 -> 輸入 byrne 按 Enter -> 出現綠字 Cheat Enabled`n" +
                "   Open console with Tab+F11 -> Type 'byrne' and press Enter -> 'Cheat Enabled'`n`n" +
                "2.【Tab + F3】全法術 (All Spells)  | 【Tab + F4】全建築 (All Buildings)  | 【Tab + F5】滿法力 (Max Mana)`n" +
                "   【Tab + F1】快速回魔 (Fast Mana Regen)  | 【Tab + F2】免費施法 (Free Spells)`n`n" +
                "3.【Alt + Q/A/Z/X】刷兵與刷建築 (Thing Cheat Menu)`n" +
                "   Hold Alt: Q = Select Object, A = Select Tribe, Z = Quantity, X = Spawn at Cursor`n`n" +
                "4.【Shift + F1】開關圖形化除錯選單 / Toggle ImGui Debug Menu`n" +
                "5.【Shift + =】遊戲快轉加速 (最高 33 倍速) / Accelerate Game Simulation (Up to 33x)`n`n" +
                "★ 提醒：打字前請切換為英文輸入法 (ENG)；筆電若無反應請加按 Fn 鍵。`n" +
                "   Tip: Switch Windows IME to English (ENG); use Fn key on laptops if F-keys are shared."
$grpHelp.Controls.Add($lblHelp)
$form.Controls.Add($grpHelp)

Update-StatusLabel

# 儲存邏輯
function Save-DevMode {
    if (-not (Test-Path $LauncherIni)) {
        $initContent = "[Launcher]`r`ndevelop_mode=0`r`n[Game]`r`n"
        [System.IO.File]::WriteAllText($LauncherIni, $initContent, [System.Text.Encoding]::UTF8)
    }

    $raw = [System.IO.File]::ReadAllText($LauncherIni, [System.Text.Encoding]::UTF8)
    $targetVal = if ($rbOn.Checked) { "1" } else { "0" }

    if ($raw -match "develop_mode\s*=\s*\d+") {
        $raw = $raw -replace "develop_mode\s*=\s*\d+", "develop_mode=$targetVal"
    } else {
        if ($raw -match "\[Launcher\]") {
            $raw = $raw -replace "\[Launcher\]", "[Launcher]`r`ndevelop_mode=$targetVal"
        } else {
            $raw = "[Launcher]`r`ndevelop_mode=$targetVal`r`n" + $raw
        }
    }

    [System.IO.File]::WriteAllText($LauncherIni, $raw, [System.Text.Encoding]::UTF8)
}

# 4. 按鈕區
$btnSave = New-Object System.Windows.Forms.Button
$btnSave.Text = "套用設定 / Apply"
$btnSave.Location = New-Object System.Drawing.Point(185, 505)
$btnSave.Size = New-Object System.Drawing.Size(185, 46)
$btnSave.BackColor = [System.Drawing.Color]::FromArgb(46, 139, 87)
$btnSave.ForeColor = [System.Drawing.Color]::White
$btnSave.Font = New-Object System.Drawing.Font("Microsoft JhengHei", 10.5, [System.Drawing.FontStyle]::Bold)
$btnSave.Add_Click({
    Save-DevMode
    $modeStr = if ($rbOn.Checked) { "【開啟】(develop_mode=1) / ENABLED" } else { "【關閉】(develop_mode=0) / DISABLED" }
    [System.Windows.Forms.MessageBox]::Show("設定已成功套用！`n目前密技模式為：$modeStr`n`nSettings applied successfully! Enjoy the game!", "儲存成功 / Saved", [System.Windows.Forms.MessageBoxButtons]::OK, [System.Windows.Forms.MessageBoxIcon]::Information)
    $form.Close()
})
$form.Controls.Add($btnSave)

$btnClose = New-Object System.Windows.Forms.Button
$btnClose.Text = "取消關閉 / Close"
$btnClose.Location = New-Object System.Drawing.Point(395, 505)
$btnClose.Size = New-Object System.Drawing.Size(165, 46)
$btnClose.Font = New-Object System.Drawing.Font("Microsoft JhengHei", 10)
$btnClose.Add_Click({
    $form.Close()
})
$form.Controls.Add($btnClose)

# 顯示視窗
[System.Windows.Forms.Application]::EnableVisualStyles()
$form.ShowDialog() | Out-Null
