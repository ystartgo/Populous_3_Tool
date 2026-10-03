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

# 建立 GUI 視窗
$form = New-Object System.Windows.Forms.Form
$form.Text = "《上帝也瘋狂 3》密技與開發者模式開關 (Develop Mode)"
$form.Size = New-Object System.Drawing.Size(530, 480)
$form.StartPosition = "CenterScreen"
$form.FormBorderStyle = [System.Windows.Forms.FormBorderStyle]::FixedDialog
$form.MaximizeBox = $false
$form.Font = New-Object System.Drawing.Font("Microsoft JhengHei", 9.5)

# 頂部狀態卡片 GroupBox
$grpStatus = New-Object System.Windows.Forms.GroupBox
$grpStatus.Text = "目前密技狀態"
$grpStatus.Location = New-Object System.Drawing.Point(20, 15)
$grpStatus.Size = New-Object System.Drawing.Size(475, 75)

$lblStatus = New-Object System.Windows.Forms.Label
$lblStatus.Location = New-Object System.Drawing.Point(20, 26)
$lblStatus.Size = New-Object System.Drawing.Size(435, 35)
$lblStatus.Font = New-Object System.Drawing.Font("Microsoft JhengHei", 12, [System.Drawing.FontStyle]::Bold)

function Update-StatusLabel {
    if ($rbOn.Checked) {
        $lblStatus.Text = "● 目前狀態：【已開啟】密技模式 (develop_mode=1)"
        $lblStatus.ForeColor = [System.Drawing.Color]::FromArgb(0, 130, 0)
    } else {
        $lblStatus.Text = "○ 目前狀態：【已關閉】普通模式 (develop_mode=0)"
        $lblStatus.ForeColor = [System.Drawing.Color]::FromArgb(180, 40, 40)
    }
}

$grpStatus.Controls.Add($lblStatus)
$form.Controls.Add($grpStatus)

# 開關切換 GroupBox
$grpSwitch = New-Object System.Windows.Forms.GroupBox
$grpSwitch.Text = "選擇密技與開發者模式 (develop_mode)"
$grpSwitch.Location = New-Object System.Drawing.Point(20, 100)
$grpSwitch.Size = New-Object System.Drawing.Size(475, 105)

$rbOn = New-Object System.Windows.Forms.RadioButton
$rbOn.Text = "【開啟】密技模式 (develop_mode=1) - 解鎖 byrne、Tab+F1~F5、Alt刷兵刷物"
$rbOn.Location = New-Object System.Drawing.Point(20, 26)
$rbOn.Size = New-Object System.Drawing.Size(445, 26)
$rbOn.Checked = $isDevMode
$rbOn.Font = New-Object System.Drawing.Font("Microsoft JhengHei", 9.5, [System.Drawing.FontStyle]::Bold)
$rbOn.Add_CheckedChanged({ Update-StatusLabel })

$rbOff = New-Object System.Windows.Forms.RadioButton
$rbOff.Text = "【關閉】普通原版模式 (develop_mode=0) - 停用作弊，回歸純淨挑戰"
$rbOff.Location = New-Object System.Drawing.Point(20, 60)
$rbOff.Size = New-Object System.Drawing.Size(445, 26)
$rbOff.Checked = (-not $isDevMode)
$rbOff.Add_CheckedChanged({ Update-StatusLabel })

$grpSwitch.Controls.Add($rbOn)
$grpSwitch.Controls.Add($rbOff)
$form.Controls.Add($grpSwitch)

# 密技使用說明 GroupBox
$grpHelp = New-Object System.Windows.Forms.GroupBox
$grpHelp.Text = "開啟後可用密技快捷鍵"
$grpHelp.Location = New-Object System.Drawing.Point(20, 215)
$grpHelp.Size = New-Object System.Drawing.Size(475, 140)

$lblHelp = New-Object System.Windows.Forms.Label
$lblHelp.Location = New-Object System.Drawing.Point(15, 22)
$lblHelp.Size = New-Object System.Drawing.Size(445, 110)
$lblHelp.Font = New-Object System.Drawing.Font("Consolas", 9.0)
$lblHelp.Text = "1. Tab + F11 跳出控制台 -> 輸入 byrne 按 Enter -> 出現 Cheat Enabled`n" +
                "2. Tab + F3 獲得全法術  |  Tab + F4 獲得全建築  |  Tab + F5 法力全滿`n" +
                "3. Alt+Q/A/Z 選兵種與部落，Alt+X 在游標處直接刷兵刷建築 (Thing Cheat)`n" +
                "4. Shift + F1 可開啟圖形化 ImGui 除錯選單`n" +
                "★ 提醒：打字前請切換為純英文輸入法 (ENG)，按鍵請先按住 Tab 再按 F 鍵。"
$grpHelp.Controls.Add($lblHelp)
$form.Controls.Add($grpHelp)

Update-StatusLabel

# 儲存邏輯
function Save-DevMode {
    if (-not (Test-Path $LauncherIni)) {
        # 若無 launcher.ini 則建立基本結構
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

# 按鈕區
$btnSave = New-Object System.Windows.Forms.Button
$btnSave.Text = "套用設定"
$btnSave.Location = New-Object System.Drawing.Point(110, 375)
$btnSave.Size = New-Object System.Drawing.Size(140, 42)
$btnSave.BackColor = [System.Drawing.Color]::FromArgb(46, 139, 87)
$btnSave.ForeColor = [System.Drawing.Color]::White
$btnSave.Font = New-Object System.Drawing.Font("Microsoft JhengHei", 10.5, [System.Drawing.FontStyle]::Bold)
$btnSave.Add_Click({
    Save-DevMode
    $modeStr = if ($rbOn.Checked) { "【開啟】(develop_mode=1)" } else { "【關閉】(develop_mode=0)" }
    [System.Windows.Forms.MessageBox]::Show("設定已成功套用！`n目前密技模式為：$modeStr`n`n現在進入遊戲即可體驗！", "儲存成功", [System.Windows.Forms.MessageBoxButtons]::OK, [System.Windows.Forms.MessageBoxIcon]::Information)
    $form.Close()
})
$form.Controls.Add($btnSave)

$btnClose = New-Object System.Windows.Forms.Button
$btnClose.Text = "取消關閉"
$btnClose.Location = New-Object System.Drawing.Point(270, 375)
$btnClose.Size = New-Object System.Drawing.Size(130, 42)
$btnClose.Font = New-Object System.Drawing.Font("Microsoft JhengHei", 10)
$btnClose.Add_Click({
    $form.Close()
})
$form.Controls.Add($btnClose)

# 顯示視窗
[System.Windows.Forms.Application]::EnableVisualStyles()
$form.ShowDialog() | Out-Null
