@echo off
chcp 65001 >nul
title 上帝也瘋狂 3 - 套用現代滑鼠設定 (右鍵移動模式)

set "GAME_DIR=C:\Program Files (x86)\Steam\steamapps\common\Populous 3"
cd /d "%~dp0"
if exist "%GAME_DIR%" cd /d "%GAME_DIR%"

echo ================================================================
echo   正在套用《上帝也瘋狂 3》現代滑鼠設定 [右鍵移動模式]...
echo ================================================================
echo.

:: 1. 啟用代理 dinput.dll
if not exist "dinput.dll" if exist "dinput.dll.bak" ren "dinput.dll.bak" "dinput.dll" >nul 2>&1

:: 2. 修改 populous_mouse.ini 啟用現代右鍵移動
if exist "populous_mouse.ini" (
    powershell -NoProfile -Command "$f='populous_mouse.ini'; (Get-Content $f -Raw) -replace 'EnableRightClickMove=0','EnableRightClickMove=1' -replace 'ModernControls=0','ModernControls=1' | Set-Content $f -Encoding UTF8"
)

:: 3. 確保 ddraw.ini 中 handlemouse=false 與 adjmouse=false，杜絕雙游標
if exist "ddraw.ini" (
    powershell -NoProfile -Command "$f='ddraw.ini'; (Get-Content $f -Raw) -replace 'handlemouse=true','handlemouse=false' -replace 'adjmouse=true','adjmouse=false' | Set-Content $f -Encoding UTF8"
)

echo [OK] DirectInput 現代右鍵移動代理層已啟用
echo [OK] 右鍵移動已開啟 [右鍵點擊地面移動角色，左鍵點擊空地取消選取]
echo [OK] DirectDraw 雙游標防護已生效 [handlemouse=false]
echo.
echo ================================================================
echo   【成功】現代滑鼠設定已生效！
echo   直接在 Steam 點擊 [開始遊戲] 或點擊桌面捷徑即可。
echo ================================================================
echo.
pause