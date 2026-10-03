@echo off
chcp 65001 >nul
cd /d "%~dp0"

set "GAME_DIR=C:\Program Files (x86)\Steam\steamapps\common\Populous 3"
if exist "%GAME_DIR%\Populous_DevMode_GUI.ps1" (
    start "" powershell -WindowStyle Hidden -NoProfile -ExecutionPolicy Bypass -File "%GAME_DIR%\Populous_DevMode_GUI.ps1"
) else (
    start "" powershell -WindowStyle Hidden -NoProfile -ExecutionPolicy Bypass -File "%~dp0Populous_DevMode_GUI.ps1"
)
exit
