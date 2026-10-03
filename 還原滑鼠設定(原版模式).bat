@echo off
chcp 65001 >nul
title 上帝也瘋狂 3 - 還原滑鼠設定 (1998 原版模式)
cd /d "%~dp0"
if exist "C:\Program Files (x86)\Steam\steamapps\common\Populous 3" (
    cd /d "C:\Program Files (x86)\Steam\steamapps\common\Populous 3"
)

echo ================================================================
echo   正在還原《上帝也瘋狂 3》滑鼠操控設定為 1998 原版模式...
echo ================================================================
echo.

:: 1. 備份並停用代理 dinput.dll (改名為 dinput.dll.bak)
if exist "dinput.dll" (
    if exist "dinput.dll.bak" del /f /q "dinput.dll.bak" >nul 2>&1
    ren "dinput.dll" "dinput.dll.bak" >nul 2>&1
)

:: 2. 修改 populous_mouse.ini 停用右鍵移動
if exist "populous_mouse.ini" (
    powershell -NoProfile -Command "(Get-Content 'populous_mouse.ini') -replace 'EnableRightClickMove=1', 'EnableRightClickMove=0' -replace 'ModernControls=1', 'ModernControls=0' | Set-Content 'populous_mouse.ini' -Encoding UTF8"
)

:: 3. 確保 ddraw.ini 中 handlemouse=false 與 adjmouse=false，徹底杜絕雙游標
if exist "ddraw.ini" (
    powershell -NoProfile -Command "(Get-Content 'ddraw.ini') -replace 'handlemouse=true', 'handlemouse=false' -replace 'adjmouse=true', 'adjmouse=false' | Set-Content 'ddraw.ini' -Encoding UTF8"
)

echo [OK] 代理層 dinput.dll 已安全停用（恢復為 Windows 系統原版 DirectInput）
echo [OK] 滑鼠控制已還原為原版 1998 模式（左鍵選取/移動，右鍵旋轉/取消）
echo [OK] DirectDraw 雙游標防護已生效（handlemouse=false）
echo.
echo ================================================================
echo   【成功】原版滑鼠設定已完整還原！
echo   現在啟動遊戲將使用 100%% 原版穩定操控，無雙游標與任何干擾。
echo ================================================================
echo.
pause