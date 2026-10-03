[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

$GameDir = "C:\Program Files (x86)\Steam\steamapps\common\Populous 3"
$DdrawIni = Join-Path $GameDir "ddraw.ini"
$D3DExe = Join-Path $GameDir "D3DPopTB.exe"
$PopExe = Join-Path $GameDir "popTB.exe"
$SaveCfg = Join-Path $GameDir "save\CONFIG00.DAT"
$MouseIni = Join-Path $GameDir "populous_mouse.ini"
$DinputDll = Join-Path $GameDir "dinput.dll"
$DinputBak = Join-Path $GameDir "dinput.dll.bak"

$isModernMouse = $true
if (Test-Path $MouseIni) {
    $mMatch = Get-Content $MouseIni | Select-String -Pattern "^EnableRightClickMove\s*=\s*(\d)" | Select-Object -First 1
    if ($mMatch -and $mMatch.Matches.Groups[1].Value -eq "0") {
        $isModernMouse = $false
    }
}

# --- 讀取當前設定 ---
$is43 = $true
if (Test-Path $DdrawIni) {
    $content = Get-Content $DdrawIni
    $match = $content | Select-String -Pattern "^maintas=(true|false)" | Select-Object -First 1
    if ($match -and $match.Matches.Groups[1].Value -eq "false") {
        $is43 = $false
    }
}

$isCleanShadow = $true
if (Test-Path $D3DExe) {
    $bytes = [System.IO.File]::ReadAllBytes($D3DExe)
    if ($bytes.Length -gt 0xa17e) {
        if ($bytes[0xa17c] -eq 0x74 -and $bytes[0xa17d] -eq 0x19) {
            $isCleanShadow = $false
        }
    }
}

$isFullscreen = $false
if (Test-Path $DdrawIni) {
    $content = Get-Content $DdrawIni
    $match = $content | Select-String -Pattern "^fullscreen=(true|false)" | Select-Object -First 1
    if ($match -and $match.Matches.Groups[1].Value -eq "true") {
        $isFullscreen = $true
    }
}

$curRes = 1024
if (Test-Path $SaveCfg) {
    $b = [System.IO.File]::ReadAllBytes($SaveCfg)
    if ($b.Length -ge 20) {
        $curRes = [System.BitConverter]::ToUInt16($b, 2)
    }
}

# --- 建立表單視窗 ---
$form = New-Object System.Windows.Forms.Form
$form.Text = "《上帝也瘋狂：開天闢地》遊戲畫面與操控設定"
$form.Size = New-Object System.Drawing.Size(520, 665)
$form.StartPosition = "CenterScreen"
$form.FormBorderStyle = [System.Windows.Forms.FormBorderStyle]::FixedDialog
$form.MaximizeBox = $false
$form.Font = New-Object System.Drawing.Font("Microsoft JhengHei", 9.5)

# 1. 畫面比例 GroupBox
$grpAspect = New-Object System.Windows.Forms.GroupBox
$grpAspect.Text = "畫面比例 (Aspect Ratio)"
$grpAspect.Location = New-Object System.Drawing.Point(20, 10)
$grpAspect.Size = New-Object System.Drawing.Size(465, 75)

$rb43 = New-Object System.Windows.Forms.RadioButton
$rb43.Text = "4:3 經典正方形比例 (推薦，文字與畫面不被橫向拉扁)"
$rb43.Location = New-Object System.Drawing.Point(15, 22)
$rb43.Size = New-Object System.Drawing.Size(440, 22)
$rb43.Checked = $is43

$rb169 = New-Object System.Windows.Forms.RadioButton
$rb169.Text = "16:9 寬螢幕拉伸 (畫面左右填滿全螢幕)"
$rb169.Location = New-Object System.Drawing.Point(15, 46)
$rb169.Size = New-Object System.Drawing.Size(440, 22)
$rb169.Checked = (-not $is43)

$grpAspect.Controls.Add($rb43)
$grpAspect.Controls.Add($rb169)
$form.Controls.Add($grpAspect)

# 2. 文字陰影 GroupBox
$grpShadow = New-Object System.Windows.Forms.GroupBox
$grpShadow.Text = "字體外觀與陰影 (Text Font & Shadow)"
$grpShadow.Location = New-Object System.Drawing.Point(20, 95)
$grpShadow.Size = New-Object System.Drawing.Size(465, 75)

$rbShadowClean = New-Object System.Windows.Forms.RadioButton
$rbShadowClean.Text = "清晰純字體模式 (推薦，去除黑影重影，字形銳利好讀)"
$rbShadowClean.Location = New-Object System.Drawing.Point(15, 22)
$rbShadowClean.Size = New-Object System.Drawing.Size(440, 22)
$rbShadowClean.Checked = $isCleanShadow

$rbShadowOrig = New-Object System.Windows.Forms.RadioButton
$rbShadowOrig.Text = "原版 3D 浮雕陰影模式 (還原 1998 原版黑底陰影)"
$rbShadowOrig.Location = New-Object System.Drawing.Point(15, 46)
$rbShadowOrig.Size = New-Object System.Drawing.Size(440, 22)
$rbShadowOrig.Checked = (-not $isCleanShadow)

$grpShadow.Controls.Add($rbShadowClean)
$grpShadow.Controls.Add($rbShadowOrig)
$form.Controls.Add($grpShadow)

# 3. 滑鼠操作模式 GroupBox (NEW!)
$grpMouse = New-Object System.Windows.Forms.GroupBox
$grpMouse.Text = "滑鼠操控習慣 (Mouse Controls)"
$grpMouse.Location = New-Object System.Drawing.Point(20, 180)
$grpMouse.Size = New-Object System.Drawing.Size(465, 75)

$rbMouseModern = New-Object System.Windows.Forms.RadioButton
$rbMouseModern.Text = "現代 RTS 模式 (推薦：左鍵選取/框選，右鍵移動小人，Shift+右鍵取消)"
$rbMouseModern.Location = New-Object System.Drawing.Point(15, 22)
$rbMouseModern.Size = New-Object System.Drawing.Size(440, 22)
$rbMouseModern.Checked = $isModernMouse

$rbMouseClassic = New-Object System.Windows.Forms.RadioButton
$rbMouseClassic.Text = "1998 原版模式 (左鍵選取兼移動，右鍵點擊取消選取)"
$rbMouseClassic.Location = New-Object System.Drawing.Point(15, 46)
$rbMouseClassic.Size = New-Object System.Drawing.Size(440, 22)
$rbMouseClassic.Checked = (-not $isModernMouse)

$grpMouse.Controls.Add($rbMouseModern)
$grpMouse.Controls.Add($rbMouseClassic)
$form.Controls.Add($grpMouse)

# 4. 顯示模式 GroupBox
$grpMode = New-Object System.Windows.Forms.GroupBox
$grpMode.Text = "顯示模式 (Display Mode)"
$grpMode.Location = New-Object System.Drawing.Point(20, 265)
$grpMode.Size = New-Object System.Drawing.Size(465, 75)

$rbWin = New-Object System.Windows.Forms.RadioButton
$rbWin.Text = "1080p 視窗化模式 (相容性佳，切換視窗方便)"
$rbWin.Location = New-Object System.Drawing.Point(15, 22)
$rbWin.Size = New-Object System.Drawing.Size(440, 22)
$rbWin.Checked = (-not $isFullscreen)

$rbFull = New-Object System.Windows.Forms.RadioButton
$rbFull.Text = "全螢幕模式 (沉浸式全螢幕遊玩)"
$rbFull.Location = New-Object System.Drawing.Point(15, 46)
$rbFull.Size = New-Object System.Drawing.Size(440, 22)
$rbFull.Checked = $isFullscreen

$grpMode.Controls.Add($rbWin)
$grpMode.Controls.Add($rbFull)
$form.Controls.Add($grpMode)

# 5. 3D 解析度 GroupBox
$grpRes = New-Object System.Windows.Forms.GroupBox
$grpRes.Text = "3D 關卡戰場渲染解析度 (In-Game Resolution)"
$grpRes.Location = New-Object System.Drawing.Point(20, 350)
$grpRes.Size = New-Object System.Drawing.Size(465, 100)

$rb1024 = New-Object System.Windows.Forms.RadioButton
$rb1024.Text = "1024 x 768 (最高畫質推薦，小人與地形最清晰)"
$rb1024.Location = New-Object System.Drawing.Point(15, 22)
$rb1024.Size = New-Object System.Drawing.Size(440, 22)
$rb1024.Checked = ($curRes -eq 1024 -or ($curRes -ne 800 -and $curRes -ne 640))

$rb800 = New-Object System.Windows.Forms.RadioButton
$rb800.Text = "800 x 600 (經典中畫質)"
$rb800.Location = New-Object System.Drawing.Point(15, 46)
$rb800.Size = New-Object System.Drawing.Size(440, 22)
$rb800.Checked = ($curRes -eq 800)

$rb640 = New-Object System.Windows.Forms.RadioButton
$rb640.Text = "640 x 480 (復古低畫質)"
$rb640.Location = New-Object System.Drawing.Point(15, 70)
$rb640.Size = New-Object System.Drawing.Size(440, 22)
$rb640.Checked = ($curRes -eq 640)

$grpRes.Controls.Add($rb1024)
$grpRes.Controls.Add($rb800)
$grpRes.Controls.Add($rb640)
$form.Controls.Add($grpRes)

# 提示文字
$lblTip = New-Object System.Windows.Forms.Label
$lblTip.Text = "★ 溫馨提醒：請在此設定解析度，切勿在遊戲內選單中切換（避免 1998 老引擎閃退）"
$lblTip.Location = New-Object System.Drawing.Point(20, 460)
$lblTip.Size = New-Object System.Drawing.Size(465, 30)
$lblTip.ForeColor = [System.Drawing.Color]::DarkSlateGray
$lblTip.Font = New-Object System.Drawing.Font("Microsoft JhengHei", 8.5)
$form.Controls.Add($lblTip)

# 儲存邏輯函數
function Save-AllSettings {
    # 1. 儲存畫面比例
    if (Test-Path $DdrawIni) {
        $text = [System.IO.File]::ReadAllText($DdrawIni)
        if ($rb43.Checked) {
            $text = $text -replace "maintas=false", "maintas=true"
        } else {
            $text = $text -replace "maintas=true", "maintas=false"
        }
        
        # 2. 儲存顯示模式
        if ($rbFull.Checked) {
            $text = $text -replace "fullscreen=false", "fullscreen=true"
            $text = $text -replace "windowed=true", "windowed=false"
        } else {
            $text = $text -replace "fullscreen=true", "fullscreen=false"
            $text = $text -replace "windowed=false", "windowed=true"
        }
        [System.IO.File]::WriteAllText($DdrawIni, $text)
    }

    # 3. 儲存文字陰影
    $exes = @($D3DExe, $PopExe)
    foreach ($exe in $exes) {
        if (Test-Path $exe) {
            $bytes = [System.IO.File]::ReadAllBytes($exe)
            if ($bytes.Length -gt 0xeb722) {
                if ($rbShadowClean.Checked) {
                    $bytes[0xa17c] = 0xeb
                    $bytes[0xa17d] = 0x40
                    $bytes[0xeb720] = 0xeb
                    $bytes[0xeb721] = 0x3b
                } else {
                    $bytes[0xa17c] = 0x74
                    $bytes[0xa17d] = 0x19
                    $bytes[0xeb720] = 0x75
                    $bytes[0xeb721] = 0x00
                }
                [System.IO.File]::WriteAllBytes($exe, $bytes)
            }
        }
    }

    # 4. 儲存滑鼠控制模式 (DirectInput RTS 右鍵移動)
    if ($rbMouseModern.Checked) {
        if (Test-Path $DinputBak -and -not (Test-Path $DinputDll)) {
            Rename-Item -Path $DinputBak -NewName "dinput.dll" -Force
        }
        if (Test-Path $MouseIni) {
            $mText = (Get-Content $MouseIni) -replace "^EnableRightClickMove\s*=.*$", "EnableRightClickMove=1"
            $mText = $mText -replace "^ModernControls\s*=.*$", "ModernControls=1"
            [System.IO.File]::WriteAllLines($MouseIni, $mText, [System.Text.Encoding]::UTF8)
        }
    } else {
        if (Test-Path $MouseIni) {
            $mText = (Get-Content $MouseIni) -replace "^EnableRightClickMove\s*=.*$", "EnableRightClickMove=0"
            $mText = $mText -replace "^ModernControls\s*=.*$", "ModernControls=0"
            [System.IO.File]::WriteAllLines($MouseIni, $mText, [System.Text.Encoding]::UTF8)
        }
    }

    # 5. 儲存 3D 解析度
    $selRes = 1024
    if ($rb800.Checked) { $selRes = 800 }
    if ($rb640.Checked) { $selRes = 640 }

    $cfgs = @($SaveCfg)
    $gdata = Join-Path $GameDir "Multiverse\GameData"
    if (Test-Path $gdata) {
        Get-ChildItem -Path $gdata -Recurse -Filter "CONFIG00.DAT" | ForEach-Object {
            $cfgs += $_.FullName
        }
    }

    foreach ($cfg in $cfgs) {
        if (Test-Path $cfg) {
            $b = [System.IO.File]::ReadAllBytes($cfg)
            if ($b.Length -eq 177) {
                $wBytes = [System.BitConverter]::GetBytes([uint16]$selRes)
                $b[2] = $wBytes[0]
                $b[3] = $wBytes[1]
                $b[18] = $wBytes[0]
                $b[19] = $wBytes[1]
                [System.IO.File]::WriteAllBytes($cfg, $b)
            }
        }
    }
}

function Launch-GameWithMouseOption {
    Save-AllSettings
    Start-Process $D3DExe -WorkingDirectory $GameDir
}

# 按鈕 1: 儲存並啟動遊戲
$btnLaunch = New-Object System.Windows.Forms.Button
$btnLaunch.Text = "儲存並啟動遊戲"
$btnLaunch.Location = New-Object System.Drawing.Point(20, 500)
$btnLaunch.Size = New-Object System.Drawing.Size(165, 44)
$btnLaunch.BackColor = [System.Drawing.Color]::FromArgb(46, 139, 87)
$btnLaunch.ForeColor = [System.Drawing.Color]::White
$btnLaunch.Font = New-Object System.Drawing.Font("Microsoft JhengHei", 10.5, [System.Drawing.FontStyle]::Bold)
$btnLaunch.Add_Click({
    Launch-GameWithMouseOption
    $form.Close()
})
$form.Controls.Add($btnLaunch)

# 按鈕 2: 僅儲存設定
$btnSave = New-Object System.Windows.Forms.Button
$btnSave.Text = "僅儲存設定"
$btnSave.Location = New-Object System.Drawing.Point(205, 500)
$btnSave.Size = New-Object System.Drawing.Size(135, 44)
$btnSave.Font = New-Object System.Drawing.Font("Microsoft JhengHei", 9.5)
$btnSave.Add_Click({
    Save-AllSettings
    [System.Windows.Forms.MessageBox]::Show("所有設定已成功儲存！", "提示", [System.Windows.Forms.MessageBoxButtons]::OK, [System.Windows.Forms.MessageBoxIcon]::Information)
})
$form.Controls.Add($btnSave)

# 按鈕 3: 關閉
$btnClose = New-Object System.Windows.Forms.Button
$btnClose.Text = "關閉"
$btnClose.Location = New-Object System.Drawing.Point(360, 500)
$btnClose.Size = New-Object System.Drawing.Size(125, 44)
$btnClose.Font = New-Object System.Drawing.Font("Microsoft JhengHei", 9.5)
$btnClose.Add_Click({
    $form.Close()
})
$form.Controls.Add($btnClose)

# 顯示視窗
[System.Windows.Forms.Application]::EnableVisualStyles()
$form.ShowDialog() | Out-Null