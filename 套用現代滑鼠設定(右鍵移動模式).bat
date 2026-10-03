@echo off
chcp 65001 >nul
title 上帝也瘋狂 3 - 套用現代滑鼠設定 (右鍵移動模式)
cd /d "%~dp0"
if exist "C:\Program Files (x86)\Steam\steamapps\common\Populous 3" (
    cd /d "C:\Program Files (x86)\Steam\steamapps\common\Populous 3"
)

echo ================================================================
echo   正在套用《上帝也瘋狂 3》現代滑鼠設定 (右鍵移動模式)...
echo ================================================================
echo.

:: 1. 啟用代理 dinput.dll
if not exist "dinput.dll" (
    if exist "dinput.dll.bak" ren "dinput.dll.bak" "dinput.dll" >nul 2>&1
)

:: 2. 修改 populous_mouse.ini 啟用右鍵移動
if exist "populous_mouse.ini" (
    powershell -NoProfile -Command "(Get-Content 'populous_mouse.ini') -replace 'EnableRightClickMove=0', 'EnableRightClickMove=1' -replace 'ModernControls=0', 'ModernControls=1' | Set-Content 'populous_mouse.ini' -Encoding UTF8"
)

:: 3. 確保 ddraw.ini 中 handlemouse=false 與 adjmouse=false，徹底杜絕雙游標
if exist "ddraw.ini" (
    powershell -NoProfile -Command "(Get-Content 'ddraw.ini') -replace 'handlemouse=true', 'handlemouse=false' -replace 'adjmouse=true', 'adjmouse=false' | Set-Content 'ddraw.ini' -Encoding UTF8"
)

echo [OK] DirectInput 現代右鍵移動代理層已啟用
echo [OK] 右鍵移動已開啟（右鍵點擊地面小人移動，Shift+右鍵原版旋轉/取消）
echo [OK] DirectDraw 雙游標防護已生效（handlemouse=false）
echo.
echo ================================================================
echo   【成功】現代滑鼠設定已生效！
echo   直接點擊桌面遊戲捷徑或在 Steam 啟動遊戲即可。
echo ================================================================
echo.
pause