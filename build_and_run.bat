@echo off
REM Get the current folder name as the game name
for %%i in ("%cd%") do set "GAME_NAME=%%~ni"
echo Building and running %GAME_NAME% Debug...
call build_debug.bat
if %ERRORLEVEL% == 0 (
    echo.
    echo Starting game...
    call run_game.bat
) else (
    echo Build failed!
    exit /b 1
)
