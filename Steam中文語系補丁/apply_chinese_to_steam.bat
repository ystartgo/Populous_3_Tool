@echo off
cd /d "%~dp0"
echo Applying Chinese Localization to Steam Populous 3...
SET STEAM_DIR=C:\Program Files (x86)\Steam\steamapps\common\Populous 3
if not exist "%STEAM_DIR%" (
    echo Error: Steam Populous 3 directory not found!
    pause
    exit /b 1
)
copy /y "%~dp0language\lang09.dat" "%STEAM_DIR%\language\lang09.dat" >nul
copy /y "%~dp0data\fenew\*.*" "%STEAM_DIR%\data\fenew\" >nul
copy /y "%~dp0data\loadlog3.dat" "%STEAM_DIR%\data\loadlog3.dat" >nul
copy /y "%~dp0fmv\p3introc.tgq" "%STEAM_DIR%\fmv\p3introc.tgq" >nul

REG ADD "HKEY_LOCAL_MACHINE\SOFTWARE\Bullfrog Productions Ltd\Populous: The Beginning" /v "Language" /t REG_DWORD /d 9 /f /reg:32 >nul 2>&1
REG ADD "HKEY_CURRENT_USER\Software\Bullfrog Productions Ltd\Populous: The Beginning" /v "Language" /t REG_DWORD /d 9 /f >nul 2>&1

echo Done! Chinese Localization has been applied to Steam.
pause
