@echo off
REM Get the current folder name as the game name
for %%i in ("%cd%") do set "GAME_NAME=%%~ni"
echo Running %GAME_NAME%...

if exist "build\debug\bin\%GAME_NAME%_debug.exe" (
    echo Starting debug version...
    cd build\debug\bin
    %GAME_NAME%_debug.exe
    cd ..\..\..
) else if exist "build\release\bin\%GAME_NAME%.exe" (
    echo Debug version not found, starting release version...
    cd build\release\bin
    %GAME_NAME%.exe
    cd ..\..\..
) else (
    echo No built executable found!
    echo Please run build_debug.bat first
)
